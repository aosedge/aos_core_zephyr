/*
 * Copyright (C) 2024 Renesas Electronics Corporation.
 * Copyright (C) 2024 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "channel.hpp"

aos::Error Channel::Init(CommunicationItf& communication, int port)
{
    mCommunication = &communication;
    mPort          = port;

    return aos::ErrorEnum::eNone;
}

aos::Error Channel::Connect()
{
    return aos::ErrorEnum::eNone;
}

aos::Error Channel::Close()
{
    return aos::ErrorEnum::eNone;
}

aos::Error Channel::Read(void* buffer, int len)
{
    aos::UniqueLock lock(mMutex);

    mReadReady = true;
    mBufferLen = len;
    mBuffer    = buffer;

    mCondVar.Wait(lock, [this] { return !mReadReady; });

    return aos::ErrorEnum::eNone;
}

aos::Error Channel::Write(void* buffer, int len)
{
    header = PrepareHeader(mPort, aos::Array<uint8_t>(reinterpret_cast<uint8_t*>(buffer), len));

    if (auto err = mCommunication->Write(&header, sizeof(header)); !err.IsNone()) {
        return err;
    }

    return mCommunication->Write(buffer, len);
}

aos::Error Channel::WaitRead(void** buffer, size_t* size)
{
    aos::UniqueLock lock(mMutex);

#ifdef CONFIG_NATIVE_APPLICATION
    auto now = aos::Time::Now();
#else
    auto now = aos::Time::Now(CLOCK_MONOTONIC);
#endif

    auto err = mCondVar.Wait(lock, now.Add(cWaitReadPeriod), [this] { return mReadReady; });
    if (!err.IsNone) {
        return err;
    }

    *buffer = mBuffer;
    *size   = mBufferLen;

    return aos::ErrorEnum::eNone;
}

aos::Error Channel::ReadDone()
{
    aos::UniqueLock lock(mMutex);

    mReadReady = false;
    mCondVar.NotifyOne();

    return aos::ErrorEnum::eNone;
}
