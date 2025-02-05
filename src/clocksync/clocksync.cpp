/*
 * Copyright (C) 2024 Renesas Electronics Corporation.
 * Copyright (C) 2024 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "clocksync.hpp"
#include "log.hpp"

namespace aos::zephyr::clocksync {

/***********************************************************************************************************************
 * Public
 **********************************************************************************************************************/

Error ClockSync::Init(ClockSyncSenderItf& sender)
{
    LOG_DBG() << "Init clock sync";

    mSender = &sender;

    auto err = mThread.Run([this](void*) {
        while (true) {
            UniqueLock lock {mMutex};

            mCondVar.Wait(lock, cSendPeriod, [this] { return mStart || mClose || mSync; });

            if (mClose) {
                return;
            }

            if (mStart) {
                mStart   = false;
                mStarted = true;
            }

            if (mSync) {
                mSync     = false;
                mSyncTime = Time::Now(CLOCK_MONOTONIC);

                if (!mSynced) {
                    mSynced = true;
                    ClockSyncNotification();
                }

                continue;
            }

            if (mSynced && abs(Time::Now(CLOCK_MONOTONIC).Sub(mSyncTime)) > cSyncTimeout) {
                LOG_WRN() << "Time is not synced";

                mSynced = false;
                ClockSyncNotification();
            }

            if (mStarted) {
                auto err = mSender->SendClockSyncRequest();
                if (!err.IsNone()) {
                    LOG_ERR() << "Error sending clock sync request: " << err;
                }
            }
        }
    });
    if (!err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    return ErrorEnum::eNone;
}

Error ClockSync::Start()
{
    LockGuard lock {mMutex};

    LOG_DBG() << "Start";

    mStart = true;
    mCondVar.NotifyOne();

    return ErrorEnum::eNone;
}

Error ClockSync::Stop()
{
    {
        LockGuard lock {mMutex};

        LOG_DBG() << "Stop";

        mClose = true;
        mCondVar.NotifyOne();
    }

    return mThread.Join();
}

Error ClockSync::Sync(const Time& time)
{
    LockGuard lock {mMutex};

    LOG_DBG() << "Sync: time = " << time;

    if (llabs(Time::Now().Sub(time)) > cMaxTimeDiff) {
        LOG_DBG() << "Set time: time = " << time;

        auto ts = time.UnixTime();

        auto ret = clock_settime(CLOCK_REALTIME, &ts);
        if (ret != 0) {
            return AOS_ERROR_WRAP(ret);
        }
    }

    mSync = true;
    mCondVar.NotifyOne();

    return ErrorEnum::eNone;
}

Error ClockSync::Subscribe(ClockSyncSubscriberItf& subscriber)
{
    LockGuard lock {mMutex};

    auto err = mConnectionSubscribers.PushBack(&subscriber);
    if (!err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    return ErrorEnum::eNone;
}

void ClockSync::Unsubscribe(ClockSyncSubscriberItf& subscriber)
{
    LockGuard lock {mMutex};

    mConnectionSubscribers.Remove(&subscriber);
}

/***********************************************************************************************************************
 * Private
 **********************************************************************************************************************/

void ClockSync::ClockSyncNotification()
{
    LOG_INF() << "Clock sync notification: synced=" << (mSynced ? "true" : "false");

    for (auto& subscriber : mConnectionSubscribers) {
        if (mSynced) {
            subscriber->OnClockSynced();
        } else {
            subscriber->OnClockUnsynced();
        }
    }
}

} // namespace aos::zephyr::clocksync
