/*
 * Copyright (C) 2024 Renesas Electronics Corporation.
 * Copyright (C) 2024 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "communicationmanager.hpp"
#include "utils/checksum.hpp"

aos::Error CommunicationManager::Init(TransportIntf& transport)
{
    mTransport = &transport;

    return aos::ErrorEnum::eNone;
}

aos::Error CommunicationManager::AddChannel(int port, ChanIntf& channel)
{
    ChannelInfo info = {port, &channel};

    return mChannels.PushBack(info);
}

aos::Error CommunicationManager::Run()
{
    return mThread.Run([this](void*) {
        while (true) {
            {
                aos::UniqueLock lock(mMutex);
                if (mClose) {
                    return aos::ErrorEnum::eNone;
                }
            }

            if (auto err = Connect(); !err.IsNone()) {
                continue;
            }

            if (err = HandleRead(); !err.IsNone()) {
                continue;
            }
        }
    });
}

aos::Error CommunicationManager::Stop()
{
    {
        aos::UniqueLock lock(mMutex);
        mClose = true;
    }

    mTransport->Close();

    return mThread.Join();
}

aos::Error CommunicationManager::Connect()
{
    if (auto err = mTransport->Connect(); !err.IsNone()) {
        return err;
    }

    for (auto& channel : mChannels) {
        auto err = channel.channel->Connect();
        if (!err.IsNone()) {
            return err;
        }
    }

    return aos::ErrorEnum::eNone;
}

aos::Error CommunicationManager::HandleRead()
{
    while (true) {
        {
            aos::UniqueLock lock(mMutex);
            if (mClose) {
                return aos::ErrorEnum::eNone;
            }
        }

        if (auto err = mTransport->Read(mHeader, cHeaderSize); !err.IsNone()) {
            return err;
        }

        AosProtocolHeader header;
        std::memcpy(&header, mHeader.Get(), cHeaderSize);

        int port = header.mPort;
        int size = header.mDataSized;

        for (auto& channel : mChannels) {
            if (channel.port == port) {
                int   readSize = 0;
                void* buffer   = nullptr;
                int   size     = 0;

                while (readSize < size) {
                    if (auto err = channel.mChannel->WaitRead(&buffer, &size); !err.IsNone()) {
                        // print log error
                        break;
                    }

                    if (auto err = mTransport->Read(buffer, size); !err.IsNone()) {
                        return err;
                    }

                    if (auto err = channel.mChannel->ReadDone(size); !err.IsNone()) {
                        // print log error
                        break;
                    }

                    readSize += size;
                }

                break;
            }
        }
    }
}