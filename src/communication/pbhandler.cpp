/*
 * Copyright (C) 2024 Renesas Electronics Corporation.
 * Copyright (C) 2024 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "pbhandler.hpp"
#include "log.hpp"

namespace aos::zephyr::communication {

/***********************************************************************************************************************
 * Public
 **********************************************************************************************************************/

template <size_t cReceiveBufferSize, size_t cSendBufferSize>
Error PBHandler<cReceiveBufferSize, cSendBufferSize>::Init(const String& name, ChannelItf& channel)
{
    LockGuard lock {mMutex};

    if (mStarted) {
        return Error(ErrorEnum::eWrongState, "PB handler already started");
    }

    mName    = name;
    mChannel = &channel;

    return ErrorEnum::eNone;
}

template <size_t cReceiveBufferSize, size_t cSendBufferSize>
Error PBHandler<cReceiveBufferSize, cSendBufferSize>::Start()
{
    LockGuard lock {mMutex};

    LOG_DBG() << "Start PB handler: name=" << mName;

    if (mStarted) {
        return Error(ErrorEnum::eWrongState, "PB handler already started");
    }

    if (auto err = mThread.Run([this](void*) { Run(); }); !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    mStarted = true;

    return ErrorEnum::eNone;
}

template <size_t cReceiveBufferSize, size_t cSendBufferSize>
Error PBHandler<cReceiveBufferSize, cSendBufferSize>::Stop()
{
    {
        LockGuard lock {mMutex};

        if (!mStarted) {
            return ErrorEnum::eNone;
        }

        LOG_DBG() << "Stop PB handler: name=" << mName;

        mStarted = false;
        mChannel->Close();
    }

    mThread.Join();

    return ErrorEnum::eNone;
}

template <size_t cReceiveBufferSize, size_t cSendBufferSize>
PBHandler<cReceiveBufferSize, cSendBufferSize>::~PBHandler()
{
    Stop();
}

/***********************************************************************************************************************
 * Protected
 **********************************************************************************************************************/

template <size_t cReceiveBufferSize, size_t cSendBufferSize>
Error PBHandler<cReceiveBufferSize, cSendBufferSize>::SendMessage(const void* message, const pb_msgdesc_t* fields)
{
    auto outStream = pb_ostream_from_buffer(
        static_cast<pb_byte_t*>(static_cast<uint8_t*>(mSendBuffer.Get()) + sizeof(AosProtobufHeader)),
        mSendBuffer.Size() - sizeof(AosProtobufHeader));
    auto header = reinterpret_cast<AosProtobufHeader*>(mSendBuffer.Get());

    if (message && fields) {
        auto status = pb_encode(&outStream, fields, message);
        if (!status) {
            return AOS_ERROR_WRAP(Error(aos::ErrorEnum::eRuntime, "failed to encode message"));
        }
    }

    header->mDataSize = outStream.bytes_written;

    auto ret = mChannel->Write(mSendBuffer.Get(), sizeof(AosProtobufHeader) + outStream.bytes_written);
    if (ret < 0) {
        return AOS_ERROR_WRAP(Error(ret, "failed to write message"));
    }

    return ErrorEnum::eNone;
}

/***********************************************************************************************************************
 * Private
 **********************************************************************************************************************/

template <size_t cReceiveBufferSize, size_t cSendBufferSize>
void PBHandler<cReceiveBufferSize, cSendBufferSize>::Run()
{
    while (true) {
        {
            LockGuard lock {mMutex};

            if (!mStarted) {
                break;
            }
        }

        if (auto err = mChannel->Connect(); !err.IsNone()) {
            aos::UniqueLock lock {mMutex};

            LOG_ERR() << "Failed to connect: name=" << mName << ", err=" << err;
            LOG_DBG() << "Reconnect in " << cReconnectPeriod / 1000000 << " ms";

            if (err = mCondVar.Wait(lock, cReconnectPeriod, [this] { return !mStarted; });
                !err.IsNone() && !err.Is(ErrorEnum::eTimeout)) {
                LOG_ERR() << "Failed to wait reconnect: name=" << mName << ", err=" << err;
            }

            continue;
        }

        OnConnect();

        AosProtobufHeader header;

        while (true) {
#if AOS_CONFIG_THREAD_STACK_USAGE
            LOG_DBG() << "Stack usage: name=" << mName << ", size=" << mThread.GetStackUsage();
#endif

            auto ret = mChannel->Read(&header, sizeof(header));
            if (ret < 0) {
                LOG_ERR() << "Failed to read channel: name=" << mName << ", ret=" << ret << ", err=" << strerror(errno);
                break;
            }

            if (static_cast<size_t>(ret) != sizeof(header)) {
                LOG_ERR() << "Wrong header size: name=" << mName << ", ret=" << ret;
                break;
            }

            if (header.mDataSize > mReceiveBuffer.Size()) {
                LOG_ERR() << "Not enough mem in receive buffer: name=" << mName << ", dataSize=" << header.mDataSize;
                continue;
            }

            ret = mChannel->Read(mReceiveBuffer.Get(), header.mDataSize);
            if (ret < 0) {
                LOG_ERR() << "Failed to read channel: name=" << mName << ", ret=" << ret << ", err=" << strerror(errno);
                break;
            }

            if (static_cast<size_t>(ret) != header.mDataSize) {
                LOG_ERR() << "Wrong data size: name=" << mName << ", ret=" << ret;
                break;
            }

            if (auto err = ReceiveMessage(Array(static_cast<uint8_t*>(mReceiveBuffer.Get()), header.mDataSize));
                !err.IsNone()) {
                LOG_ERR() << "Receive message error: name=" << mName << ", err=" << err;
                continue;
            }
        }

        OnDisconnect();
    }
}

} // namespace aos::zephyr::communication
