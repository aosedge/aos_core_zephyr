/*
 * Copyright (C) 2024 Renesas Electronics Corporation.
 * Copyright (C) 2024 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SUBSCRIBERSTUB_HPP_
#define SUBSCRIBERSTUB_HPP_

#include <condition_variable>
#include <mutex>

#include "clocksync/clocksync.hpp"

class SubscriberStub : public aos::zephyr::clocksync::ClockSyncSubscriberItf {
public:
    void OnClockSynced() override
    {
        std::lock_guard<std::mutex> lock {mMutex};

        mEventReceived = true;
        mSynced        = true;
        mCV.notify_one();
    }

    void OnClockUnsynced() override
    {
        std::lock_guard<std::mutex> lock {mMutex};

        mEventReceived = true;
        mSynced        = false;
        mCV.notify_one();
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

    bool IsSynced() const
    {
        std::lock_guard<std::mutex> lock {mMutex};

        return mSynced;
    }

private:
    std::condition_variable mCV;
    mutable std::mutex      mMutex;
    bool                    mSynced        = false;
    bool                    mEventReceived = false;
};

#endif
