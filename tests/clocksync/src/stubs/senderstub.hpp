/*
 * Copyright (C) 2024 Renesas Electronics Corporation.
 * Copyright (C) 2024 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SENDERSTUB_HPP_
#define SENDERSTUB_HPP_

#include <condition_variable>
#include <mutex>

#include "clocksync/clocksync.hpp"

class SenderStub : public aos::zephyr::clocksync::ClockSyncSenderItf {
public:
    aos::Error SendClockSyncRequest() override
    {
        std::lock_guard<std::mutex> lock {mMutex};

        mSyncRequest   = true;
        mEventReceived = true;

        mCV.notify_one();

        return aos::ErrorEnum::eNone;
    }

    aos::Error WaitEvent(const std::chrono::duration<double> timeout)
    {
        std::unique_lock<std::mutex> lock {mMutex};

        if (!mCV.wait_for(lock, timeout, [&] { return mEventReceived; })) {
            return aos::ErrorEnum::eTimeout;
        }

        mEventReceived = false;

        return aos::ErrorEnum::eNone;
    }

    bool IsSyncRequest() const
    {
        std::lock_guard<std::mutex> lock {mMutex};
        return mSyncRequest;
    }

    void Clear()
    {
        std::lock_guard<std::mutex> lock {mMutex};

        mEventReceived = false;
        mSyncRequest   = false;
    }

private:
    std::condition_variable mCV;
    mutable std::mutex      mMutex;
    bool                    mEventReceived = false;
    bool                    mSyncRequest   = false;
};

#endif
