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

    auto err = mThread.Run([this](void*) { Run(); });
    if (!err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    mStarted = true;

    return ErrorEnum::eNone;
}

template <size_t cReceiveBufferSize, size_t cSendBufferSize>
Error PBHandler<cReceiveBufferSize, cSendBufferSize>::Stop()
{
    LOG_DBG() << "Stop PB handler: name=" << mName;

    {
        LockGuard lock {mMutex};

        if (!mStarted) {
            return ErrorEnum::eNone;
        }

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

        auto err = mChannel->Connect();
        if (!err.IsNone()) {
            LOG_ERR() << "Failed to connect: name=" << mName << ", err=" << err;
            continue;
        }

        OnConnect();

        AosProtobufHeader header;

        while (true) {
            auto ret = mChannel->Read(&header, sizeof(header));
            if (ret < 0) {
                LOG_ERR() << "Failed to read channel: name=" << mName << ", ret=" << ret << ", err=" << strerror(errno);
                break;
            }

            if (header.mDataSize > mReceiveBuffer.Size()) {
                LOG_ERR() << "Not enough mem in receive buffer: name=" << mName;
                continue;
            }

            if ((ret = mChannel->Read(mReceiveBuffer.Get(), header.mDataSize)) < 0) {
                LOG_ERR() << "Failed to read channel: name=" << mName << ", ret=" << ret << ", err=" << strerror(errno);
                break;
            }

            auto err = ReceiveMessage(Array(static_cast<uint8_t*>(mReceiveBuffer.Get()), header.mDataSize));

            if (!err.IsNone()) {
                LOG_ERR() << "Receive message error: name=" << mName << ", err=" << err;
                continue;
            }
        }

        OnDisconnect();
    }
}

} // namespace aos::zephyr::communication
