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

    return ErrorEnum::eNone;
}

Error ChannelManager::Start()
{
    LockGuard lock {mMutex};

    LOG_DBG() << "Start channel manager";

    return Run();
}

Error ChannelManager::Stop()
{
    {
        LOG_DBG() << "Stop channel manager";

        LockGuard lock {mMutex};

        if (mTransport->IsOpened()) {
            if (auto err = mTransport->Close(); !err.IsNone()) {
                LOG_ERR() << "Failed to close transport: err=" << err;
            }
        }

        mClose = true;
        mCondVar.NotifyAll();
    }

    return mThread.Join();
}

RetWithError<ChannelItf*> ChannelManager::CreateChannel(uint32_t port)
{
    LockGuard lock {mMutex};

    auto findChannel = mChannels.At(port);
    if (findChannel.mError.IsNone()) {
        return {findChannel.mValue.Get(), ErrorEnum::eNone};
    }

    LOG_DBG() << "Create channel: port=" << port;

    auto channel = MakeShared<Channel>(&mChanAllocator, static_cast<CommunicationItf*>(this), port);

    if (auto err = mChannels.Set(port, channel); !err.IsNone()) {
        return {nullptr, err};
    }

    mCondVar.NotifyAll();

    return {channel.Get(), ErrorEnum::eNone};
}

Error ChannelManager::DeleteChannel(uint32_t port)
{
    LockGuard lock {mMutex};

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

int ChannelManager::Write(uint32_t port, const void* data, size_t size)
{
    auto header = PrepareHeader(port, Array<uint8_t>(reinterpret_cast<const uint8_t*>(data), size));
    if (!header.mError.IsNone()) {
        LOG_ERR() << "Failed to prepare header: err=" << header.mError.Message();

        return -EINVAL;
    }

    LockGuard lock {sWriteMutex};

    if (auto err = WriteTransport(&header.mValue, sizeof(AosProtocolHeader)); !err.IsNone()) {
        LOG_ERR() << "Failed to write header: err=" << err;

        return -EIO;
    }

    if (auto err = WriteTransport(data, size); !err.IsNone()) {
        LOG_ERR() << "Failed to write header: err=" << err;

        return -EIO;
    }

    return size;
}

bool ChannelManager::IsConnected() const
{
    LockGuard lock {mMutex};

    return mTransport->IsOpened();
}

/***********************************************************************************************************************
 * Private
 **********************************************************************************************************************/

Error ChannelManager::TryConnect()
{
    LockGuard lock {mMutex};

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
                LockGuard lock {mMutex};

                if (mClose) {
                    return ErrorEnum::eNone;
                }
            }

            if (auto err = TryConnect(); !err.IsNone()) {
                LOG_ERR() << "Transport connect error: err=" << err;
                LOG_DBG() << "Reconnect in " << cReconnectPeriod / 1000000 << " ms";

                if (err = WaitTimeout(); !err.IsNone()) {
                    LOG_ERR() << "Failed to wait timeout: err=" << err;
                }

                continue;
            }

            mCondVar.NotifyAll();

            if (auto err = HandleRead(); !err.IsNone()) {
                LOG_ERR() << "Failed to handle read: err=" << err;
            }

            mTransport->Close();

            CloseChannels();

            if (auto err = WaitTimeout(); !err.IsNone()) {
                LOG_ERR() << "Failed to wait timeout: err=" << err;
            }
        }
    });
}

Error ChannelManager::WaitTimeout()
{
    UniqueLock lock {mMutex};

    if (auto err = mCondVar.Wait(lock, cReconnectPeriod, [this] { return mClose; });
        !err.IsNone() && !err.Is(ErrorEnum::eTimeout)) {
        return AOS_ERROR_WRAP(err);
    }

    return ErrorEnum::eNone;
}

Error ChannelManager::HandleRead()
{
    while (true) {
        {
            UniqueLock lock {mMutex};

            mCondVar.Wait(lock, [this] { return mChannels.Size() > 0 || mClose; });
            if (mClose) {
                return ErrorEnum::eNone;
            }
        }

        AosProtocolHeader header;

        if (auto err = ReadTransport(&header, sizeof(AosProtocolHeader)); !err.IsNone()) {
            return err;
        }

        if (header.mDataSize > cReadBufferSize) {
            return {ErrorEnum::eRuntime, "not enough memory in read buffer"};
        }

        if (auto err = ReadTransport(mTmpReadBuffer, header.mDataSize); !err.IsNone()) {
            return err;
        }

        if (auto err = ProcessData(header); !err.IsNone()) {
            LOG_ERR() << "Failed to process data: err=" << err;
        }
    }
}

Error ChannelManager::ProcessData(const AosProtocolHeader& header)
{
    LockGuard lock {mMutex};

    LOG_DBG() << "Process data: port=" << header.mPort << " size=" << header.mDataSize;

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

        size = Min(size, static_cast<size_t>(header.mDataSize - processedSize));

        memcpy(buffer, mTmpReadBuffer + processedSize, size);

        if (err = channel->ReadDone(size); !err.IsNone()) {
            return err;
        }

        processedSize += size;
    }

    return ErrorEnum::eNone;
}

Error ChannelManager::ReadTransport(void* buffer, size_t size)
{
    size_t read = 0;

    LOG_DBG() << "Read transport: size=" << size;

    while (read < size) {
        auto ret = mTransport->Read(&reinterpret_cast<uint8_t*>(buffer)[read], size - read);
        if (ret < 0) {
            return AOS_ERROR_WRAP(ret);
        }

        read += ret;
    }

    if (read != size) {
        return AOS_ERROR_WRAP(Error(ErrorEnum::eFailed, "read size mismatch"));
    }

    return ErrorEnum::eNone;
}

Error ChannelManager::WriteTransport(const void* buffer, size_t size)
{
    size_t written = 0;

    while (written < size) {
        auto ret = mTransport->Write(&reinterpret_cast<const uint8_t*>(buffer)[written], size - written);
        if (ret < 0) {
            return AOS_ERROR_WRAP(ret);
        }

        written += ret;
    }

    if (written != size) {
        return AOS_ERROR_WRAP(Error(ErrorEnum::eFailed, "write size mismatch"));
    }

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

RetWithError<AosProtocolHeader> ChannelManager::PrepareHeader(uint32_t port, const Array<uint8_t>& data)
{
    AosProtocolHeader header;

    header.mPort     = port;
    header.mDataSize = data.Size();

    auto ret = zephyr::utils::CalculateSha256(data);
    if (!ret.mError.IsNone()) {
        return {header, AOS_ERROR_WRAP(ret.mError)};
    }

    Array<uint8_t>(reinterpret_cast<uint8_t*>(header.mCheckSum), cSHA256Size) = ret.mValue;

    return header;
}

} // namespace aos::zephyr::communication
