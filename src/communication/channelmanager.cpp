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

/***********************************************************************************************************************
 * Public
 **********************************************************************************************************************/

Mutex ChannelManager::sWriteMutex;

Error ChannelManager::Init(TransportItf& transport)
{
    LOG_INF() << "Init channel manager";

    mTransport = &transport;

    return Run();
}

RetWithError<ChannelItf*> ChannelManager::CreateChannel(uint32_t port)
{
    UniqueLock lock {mMutex};

    auto findChannel = mChannels.At(port);
    if (findChannel.mError.IsNone()) {
        return {findChannel.mValue.Get(), ErrorEnum::eNone};
    }

    LOG_DBG() << "Create channel: port=" << port;

    auto channel = aos::MakeShared<Channel>(&mChanAllocator, static_cast<CommunicationItf*>(this), port);

    if (auto err = mChannels.Set(port, channel); !err.IsNone()) {
        return {nullptr, err};
    }

    mCondVar.NotifyAll();

    return {channel.Get(), ErrorEnum::eNone};
}

Error ChannelManager::DeleteChannel(uint32_t port)
{
    UniqueLock lock {mMutex};

    LOG_DBG() << "Delete channel: port=" << port;

    return mChannels.Remove(port);
}

Error ChannelManager::Connect()
{
    UniqueLock lock {mMutex};

    LOG_DBG() << "Connect channel manager";

    mCondVar.Wait(lock, [this] { return mTransport->IsOpened() || mClose; });
    if (mClose) {
        return ErrorEnum::eRuntime;
    }

    LOG_DBG() << "Channel manager connected";

    return ErrorEnum::eNone;
}

Error ChannelManager::Close()
{
    {
        UniqueLock lock {mMutex};

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
        LOG_ERR() << "Failed to prepare header: err=" << header.mError.Message();

        return -EINVAL;
    }

    aos::UniqueLock lock {sWriteMutex};

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

/***********************************************************************************************************************
 * Private
 **********************************************************************************************************************/

Error ChannelManager::TryConnect()
{
    UniqueLock lock {mMutex};

    if (mTransport->IsOpened()) {
        return ErrorEnum::eNone;
    }

    if (auto err = mTransport->Open(); !err.IsNone()) {
        return err;
    }

    return ErrorEnum::eNone;
}

Error ChannelManager::Run()
{
    return mThread.Run([this](void*) {
        while (true) {
            {
                aos::UniqueLock lock {mMutex};
                if (mClose) {
                    return aos::Error(aos::ErrorEnum::eNone);
                }
            }

            if (auto err = TryConnect(); !err.IsNone()) {
                LOG_ERR() << "Failed to connect: err=" << err.Message();

                if (err = WaitTimeout(); !err.IsNone()) {
                    return err;
                }

                continue;
            }

            mCondVar.NotifyAll();

            if (auto err = HandleRead(); !err.IsNone()) {
                LOG_ERR() << "Failed to handle read: err=" << err.Message();
            }

            CloseChannels();

            if (auto err = WaitTimeout(); !err.IsNone()) {
                return err;
            }
        }
    });
}

Error ChannelManager::WaitTimeout()
{
    aos::UniqueLock lock {mMutex};

    if (auto err = mCondVar.Wait(lock, cReconnectPeriod, [this] { return mClose; }); !err.IsNone()) {
        return err;
    }

    return ErrorEnum::eNone;
}

Error ChannelManager::HandleRead()
{
    while (true) {
        {
            aos::UniqueLock lock {mMutex};

            mCondVar.Wait(lock, [this] { return mChannels.Size() > 0 || mClose; });
            if (mClose) {
                return ErrorEnum::eNone;
            }
        }

        AosProtocolHeader header;

        if (auto err = ReadFromTransport(header); !err.IsNone()) {
            return err;
        }

        if (auto err = ProcessData(header); !err.IsNone()) {
            LOG_ERR() << "Failed to process data: err=" << err.Message();
        }
    }
}

Error ChannelManager::ReadFromTransport(AosProtocolHeader& header)
{
    if (auto ret = mTransport->Read(&header, sizeof(AosProtocolHeader));
        ret < static_cast<int>(sizeof(AosProtocolHeader))) {
        return {ret, "failed to read header"};
    }

    if (header.mDataSize > cReadBufferSize) {
        return {ErrorEnum::eRuntime, "not enough memory in read buffer"};
    }

    LOG_DBG() << "Read channel: port=" << header.mPort << " size=" << header.mDataSize;

    size_t totalRead = 0;

    while (totalRead < header.mDataSize) {
        size_t readSize = header.mDataSize - totalRead;

        if (auto ret = mTransport->Read(mTmpReadBuffer + totalRead, readSize); ret < static_cast<int>(readSize)) {
            return {ret, "failed to read data into local buffer"};
        }

        totalRead += readSize;
    }

    return ErrorEnum::eNone;
}

Error ChannelManager::ProcessData(const AosProtocolHeader& header)
{
    UniqueLock lock {mMutex};

    LOG_DBG() << "Read channel: port=" << header.mPort << " size=" << header.mDataSize;

    auto [channel, err] = mChannels.At(header.mPort);
    if (!err.IsNone()) {
        return err;
    }

    size_t processedSize = 0;

    while (processedSize < header.mDataSize) {
        void*  buffer = nullptr;
        size_t size   = 0;

        if (err = channel->WaitRead(&buffer, &size); !err.IsNone()) {
            return err;
        }

        LOG_DBG() << "Request read channel: port=" << header.mPort << " size=" << size;

        size = aos::Min(size, static_cast<size_t>(header.mDataSize - processedSize));

        memcpy(buffer, mTmpReadBuffer + processedSize, size);

        if (err = channel->ReadDone(size); !err.IsNone()) {
            return err;
        }

        processedSize += size;
    }

    LOG_DBG() << "Read channel done: port=" << header.mPort << " size=" << header.mDataSize;

    return ErrorEnum::eNone;
}

void ChannelManager::CloseChannels()
{
    LOG_DBG() << "Close channels";

    // cppcheck-suppress unusedVariable
    for (auto& [_, channel] : mChannels) {
        channel->Close();
    }
}

aos::RetWithError<AosProtocolHeader> ChannelManager::PrepareHeader(uint32_t port, const aos::Array<uint8_t>& data)
{
    AosProtocolHeader header;
    header.mPort     = port;
    header.mDataSize = data.Size();

    auto ret = aos::zephyr::utils::CalculateSha256(data);
    if (!ret.mError.IsNone()) {
        return {header, AOS_ERROR_WRAP(ret.mError)};
    }

    aos::Array<uint8_t>(reinterpret_cast<uint8_t*>(header.mCheckSum), aos::cSHA256Size) = ret.mValue;

    return header;
}

} // namespace aos::zephyr::communication
