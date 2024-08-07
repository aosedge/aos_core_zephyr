/*
 * Copyright (C) 2024 Renesas Electronics Corporation.
 * Copyright (C) 2024 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <aos/common/tools/timer.hpp>
#include <aosprotocol.h>

#include "channel.hpp"
#include "channelmanager.hpp"
#include "log.hpp"

#include "utils/checksum.hpp"

namespace aos::zephyr::communication {

Mutex ChannelManager::sWriteMutex;

Error ChannelManager::Init(TransportItf& transport)
{
    LOG_INF() << "Init channel manager";

    mTransport = &transport;

    return Run();
}

RetWithError<ChannelItf*> ChannelManager::CreateChannel(uint32_t port)
{
    UniqueLock lock(mMutex);

    auto findChannel = mChannels.Find([&port](const ChannelInfo& chInfo) { return chInfo.mPort == port; });

    if (findChannel.mError.IsNone()) {
        return {findChannel.mValue->mChannel.Get(), ErrorEnum::eNone};
    }

    LOG_DBG() << "Create channel: port=" << port;

    ChannelInfo info
        = {port, info.mChannel = aos::MakeShared<Channel>(&mChanAllocator, static_cast<CommunicationItf*>(this), port)};

    if (auto err = mChannels.PushBack(info); !err.IsNone()) {
        return {nullptr, err};
    }

    mCondVar.NotifyAll();

    return {info.mChannel.Get(), ErrorEnum::eNone};
}

Error ChannelManager::DeleteChannel(uint32_t port)
{
    UniqueLock lock(mMutex);

    LOG_DBG() << "Delete channel: port=" << port;

    for (auto it = mChannels.begin(); it != mChannels.end(); ++it) {
        if (it->mPort == port) {
            mChannels.Remove(it);

            return ErrorEnum::eNone;
        }
    }

    return ErrorEnum::eNotFound;
}

Error ChannelManager::Connect()
{
    UniqueLock lock(mMutex);

    LOG_DBG() << "Connect channel manager";

    // if (mTransport->IsOpened()) {
    //     return ErrorEnum::eNone;
    // }

    // if (auto err = mTransport->Open(); !err.IsNone()) {
    //     return err;
    // }

    mCondVar.Wait(lock, [this] { return mTransport->IsOpened() || mClose; });
    if (mClose) {
        return ErrorEnum::eRuntime;
    }

    return ErrorEnum::eNone;
}

Error ChannelManager::TryConnect()
{
    UniqueLock lock(mMutex);

    if (mTransport->IsOpened()) {
        return ErrorEnum::eNone;
    }

    if (auto err = mTransport->Open(); !err.IsNone()) {
        return err;
    }

    return ErrorEnum::eNone;
}

Error ChannelManager::Close()
{
    {
        UniqueLock lock(mMutex);

        LOG_DBG() << "Close channel manager";

        mClose = true;
        mCondVar.NotifyAll();

        if (!mTransport->IsOpened()) {
            return ErrorEnum::eNone;
        }

        if (auto err = mTransport->Close(); !err.IsNone()) {
            return err;
        }
    }

    return mThread.Join();
}

int ChannelManager::Write(uint32_t port, const void* data, size_t size)
{
    LOG_DBG() << "Write channel: port=" << port << " size=" << size;

    auto header = PrepareHeader(port, aos::Array<uint8_t>(reinterpret_cast<const uint8_t*>(data), size));
    if (!header.mError.IsNone()) {
        LOG_ERR() << "Failed to prepare header: error=" << header.mError.Message();

        return -EINVAL;
    }

    aos::UniqueLock lock(sWriteMutex);

    if (auto ret = mTransport->Write(&header.mValue, sizeof(AosProtocolHeader));
        ret < static_cast<int>(sizeof(AosProtocolHeader))) {
        return ret;
    }

    return mTransport->Write(data, size);
}

bool ChannelManager::IsConnected() const
{
    return mTransport->IsOpened();
}

Error ChannelManager::Run()
{
    return mThread.Run([this](void*) {
        while (true) {
            {
                aos::UniqueLock lock(mMutex);
                if (mClose) {
                    return aos::ErrorEnum::eNone;
                }
            }

            if (auto err = TryConnect(); !err.IsNone()) {
                LOG_ERR() << "Failed to connect: error=" << err.Message();

                continue;
            }

            if (auto err = HandleRead(); !err.IsNone()) {
                LOG_ERR() << "Failed to handle read: error=" << err.Message();
            }

            CloseChannels();

            {
                aos::UniqueLock lock(mMutex);
                if (mClose) {
                    return aos::ErrorEnum::eNone;
                }
            }

            sleep(cReconnectPeriod);
        }
    });
}

Error ChannelManager::HandleRead()
{
    while (true) {
        {
            aos::UniqueLock lock(mMutex);

            mCondVar.Wait(lock, [this] { return !mChannels.IsEmpty() || mClose; });

            if (mClose) {
                return ErrorEnum::eNone;
            }
        }

        if (auto ret = mTransport->Read(mHeader.Get(), sizeof(AosProtocolHeader));
            ret < static_cast<int>(sizeof(AosProtocolHeader))) {
            return {ErrorEnum::eRuntime, "Failed to read header"};
        }

        AosProtocolHeader header;
        memcpy(&header, mHeader.Get(), sizeof(AosProtocolHeader));

        uint32_t port    = header.mPort;
        uint32_t msgSize = header.mDataSize;

        UniqueLock lock(mMutex);

        for (auto& channel : mChannels) {
            if (channel.mPort == port) {
                size_t readSize = 0;
                void*  buffer   = nullptr;
                size_t size     = 0;

                while (readSize < msgSize) {
                    if (auto err = channel.mChannel->WaitRead(&buffer, &size); !err.IsNone()) {
                        LOG_ERR() << "Failed to wait read: error=" << err.Message();

                        break;
                    }

                    lock.Unlock();

                    size = aos::Min(size, static_cast<size_t>(msgSize));

                    if (auto ret = mTransport->Read(buffer, size); ret < static_cast<int>(size)) {
                        return {ErrorEnum::eRuntime, "Failed to read data"};
                    }

                    lock.Lock();

                    if (auto err = channel.mChannel->ReadDone(size); !err.IsNone()) {
                        LOG_ERR() << "Failed to read done: error=" << err.Message();

                        break;
                    }

                    readSize += size;
                }

                break;
            }
        }
    }
}

void ChannelManager::CloseChannels()
{
    LOG_DBG() << "Close channels";

    for (auto& channel : mChannels) {
        channel.mChannel->Close();
    }
}

aos::RetWithError<AosProtocolHeader> ChannelManager::PrepareHeader(uint32_t port, const aos::Array<uint8_t>& data)
{
    AosProtocolHeader header;
    header.mPort     = port;
    header.mDataSize = data.Size();

    auto ret = CalculateSha256(data);
    if (!ret.mError.IsNone()) {
        return {header, AOS_ERROR_WRAP(ret.mError)};
    }

    aos::Array<uint8_t>(reinterpret_cast<uint8_t*>(header.mCheckSum), aos::cSHA256Size) = ret.mValue;

    return header;
}

} // namespace aos::zephyr::communication
