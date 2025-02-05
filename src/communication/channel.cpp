/*
 * Copyright (C) 2024 Renesas Electronics Corporation.
 * Copyright (C) 2024 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "channel.hpp"
#include "log.hpp"

namespace aos::zephyr::communication {

Channel::Channel(CommunicationItf* communication, int port)
    : mCommunication(communication)
    , mPort(port)
{
}

Error Channel::Connect()
{
    LockGuard lock {mMutex};

    LOG_DBG() << "Connect channel: port=" << mPort;

    if (auto err = mCommunication->Connect(); !err.IsNone()) {
        return err;
    }

    mClose = false;

    return ErrorEnum::eNone;
}

Error Channel::Close()
{
    LockGuard lock {mMutex};

    LOG_DBG() << "Close channel: port=" << mPort;

    mClose = true;
    mCondVar.NotifyAll();

    return ErrorEnum::eNone;
}

int Channel::Read(void* buffer, size_t size)
{
    UniqueLock lock {mMutex};

    LOG_DBG() << "Read channel: port=" << mPort << " size=" << size;

    mBufferLen = 0;

    while (mBufferLen < size) {
        mReadReady = true;
        mBuffer    = reinterpret_cast<uint8_t*>(buffer) + mBufferLen;
        mBufferLen = size - mBufferLen;

        mCondVar.NotifyAll();

        auto err = mCondVar.Wait(lock, [this] { return !mReadReady || mClose; });
        if (!err.IsNone()) {
            return err.Errno();
        }

        if (mClose) {
            return -ECONNRESET;
        }
    }

    LOG_DBG() << "Read channel done: port=" << mPort << " size=" << size;

    return static_cast<int>(size);
}

int Channel::Write(const void* buffer, size_t size)
{
    LockGuard lock {mMutex};

    LOG_DBG() << "Write channel: port=" << mPort << " size=" << size;

    return mCommunication->Write(mPort, buffer, size);
}

Error Channel::WaitRead(void** buffer, size_t* size)
{
    UniqueLock lock {mMutex};

    LOG_DBG() << "Wait read: port=" << mPort;

    auto err = mCondVar.Wait(lock, cWaitReadPeriod, [this] { return mReadReady || mClose; });
    if (!err.IsNone()) {
        return err;
    }

    if (mClose) {
        return {aos::ErrorEnum::eRuntime, "channel is closed"};
    }

    *buffer = mBuffer;
    *size   = mBufferLen;

    return ErrorEnum::eNone;
}

Error Channel::ReadDone(size_t size)
{
    LockGuard lock {mMutex};

    LOG_DBG() << "Read done: port=" << mPort << " size=" << size;

    mReadReady = false;
    mBufferLen = size;

    mCondVar.NotifyAll();

    return ErrorEnum::eNone;
}

bool Channel::IsConnected() const
{
    LockGuard lock {mMutex};

    return mCommunication->IsConnected();
}

} // namespace aos::zephyr::communication
