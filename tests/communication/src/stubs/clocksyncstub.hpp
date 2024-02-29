/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CLOCKSYNCSTUB_HPP_
#define CLOCKSYNCSTUB_HPP_

#include <condition_variable>
#include <mutex>

#include "clocksync/clocksync.hpp"

class ClockSyncStub : public ClockSyncItf {
public:
    aos::Error Start() override
    {
        std::lock_guard<std::mutex> lock(mMutex);

        mStarted       = true;
        mEventReceived = true;
        mCV.notify_one();

        return aos::ErrorEnum::eNone;
    }

    aos::Error Sync(const aos::Time& time) override
    {
        std::lock_guard<std::mutex> lock(mMutex);

        mSyncTime      = time;
        mEventReceived = true;
        mCV.notify_one();

        return aos::ErrorEnum::eNone;
    }

    bool GetStarted() const
    {
        std::lock_guard<std::mutex> lock(mMutex);
        return mStarted;
    }

    aos::Time GetSyncTime() const
    {
        std::lock_guard<std::mutex> lock(mMutex);
        return mSyncTime;
    }

    aos::Error WaitEvent(const std::chrono::duration<double> timeout)
    {
        std::unique_lock<std::mutex> lock(mMutex);

        if (!mCV.wait_for(lock, timeout, [&] { return mEventReceived; })) {
            return aos::ErrorEnum::eTimeout;
        }

        mEventReceived = false;

        return aos::ErrorEnum::eNone;
    }

    void Clear()
    {
        std::lock_guard<std::mutex> lock(mMutex);

        mEventReceived = false;
        mStarted       = false;
        mSyncTime      = aos::Time();
    }

private:
    std::condition_variable mCV;
    mutable std::mutex      mMutex;
    bool                    mEventReceived = false;
    bool                    mStarted       = false;
    aos::Time               mSyncTime;
};

#endif
