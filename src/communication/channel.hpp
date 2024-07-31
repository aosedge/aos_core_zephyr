/*
 * Copyright (C) 2024 Renesas Electronics Corporation.
 * Copyright (C) 2024 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CHANNEL_HPP_
#define CHANNEL_HPP_

#include <aos/common/tools/thread.hpp>

#include "types.hpp"

class Channel : public ChanIntf, public AosProtocol {
public:
    aos::Error Init(CommunicationItf& communication, int port);
    aos::Error Connect() override;
    aos::Error Read(void* buffer, int len) override;
    aos::Error Write(void* buffer, int len) override;
    aos::Error WaitRead(void** buffer, size_t* size) override;
    aos::Error ReadDone() override;
    aos::Error Close() override;

private:
    constexpr static int  cMaxBufferLen   = 4096;
    static constexpr auto cWaitReadPeriod = 3 * aos::Time::cSeconds;

    CommunicationItf*        mCommunication;
    int                      mPort;
    bool                     mReadReady {};
    void*                    mBuffer;
    int                      mBufferLen;
    aos::Mutex               mMutex;
    aos::ConditionalVariable mCondVar;
};

#endif // OPENCHANNEL_HPP_