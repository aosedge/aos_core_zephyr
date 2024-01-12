/*
 * Copyright (C) 2024 Renesas Electronics Corporation.
 * Copyright (C) 2024 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SENDERMOCK_HPP_
#define SENDERMOCK_HPP_

#include <condition_variable>
#include <mutex>

#include "clocksync/clocksync.hpp"

class SenderMock : public ClockSyncSenderItf {
public:
    aos::Error SendClockSyncRequest() override
    {
        std::lock_guard<std::mutex> lock(mMutex);

        mSyncRequest   = true;
        mEventReceived = true;

        mCV.notify_one();

        return aos::ErrorEnum::eNone;
    }

    void ClockSynced() override
    {
        std::lock_guard<std::mutex> lock(mMutex);

        mSynced        = true;
        mEventReceived = true;

        mCV.notify_one();
    }

    void ClockUnsynced() override
    {
        std::lock_guard<std::mutex> lock(mMutex);

        mSynced        = false;
        mEventReceived = true;

        mCV.notify_one();
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

    bool IsSyncRequest() const
    {
        std::lock_guard<std::mutex> lock(mMutex);
        return mSyncRequest;
    }

    bool IsSynced() const
    {
        std::lock_guard<std::mutex> lock(mMutex);
        return mSynced;
    }

    void Clear()
    {
        std::lock_guard<std::mutex> lock(mMutex);

        mEventReceived = false;
        mSyncRequest   = false;
    }

private:
    std::condition_variable mCV;
    mutable std::mutex      mMutex;
    bool                    mEventReceived = false;
    bool                    mSyncRequest   = false;
    bool                    mSynced        = false;
};

#endif
