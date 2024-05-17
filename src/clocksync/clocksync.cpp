/*
 * Copyright (C) 2024 Renesas Electronics Corporation.
 * Copyright (C) 2024 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "clocksync.hpp"
#include "log.hpp"

/***********************************************************************************************************************
 * Public
 **********************************************************************************************************************/

aos::Error ClockSync::Init(ClockSyncSenderItf& sender)
{
    LOG_DBG() << "Init clock sync";

    mSender = &sender;

    auto err = mThread.Run([this](void*) {
        while (true) {
            aos::UniqueLock lock(mMutex);

#ifdef CONFIG_NATIVE_APPLICATION
            auto now = aos::Time::Now();
#else
            auto now = aos::Time::Now(CLOCK_MONOTONIC);
#endif

            mCondVar.Wait(lock, now.Add(cSendPeriod), [this] { return mStart || mClose || mSync; });

            if (mClose) {
                return;
            }

            if (mStart) {
                mStart   = false;
                mStarted = true;
            }

            if (mSync) {
                mSync     = false;
                mSyncTime = aos::Time::Now(CLOCK_MONOTONIC);

                if (!mSynced) {
                    mSynced = true;
                    mSender->ClockSynced();
                }

                continue;
            }

            if (mSynced && abs(aos::Time::Now(CLOCK_MONOTONIC).Sub(mSyncTime)) > cSyncTimeout) {
                LOG_WRN() << "Time is not synced";

                mSynced = false;
                mSender->ClockUnsynced();
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

    return aos::ErrorEnum::eNone;
}

aos::Error ClockSync::Start()
{
    aos::LockGuard lock(mMutex);

    LOG_DBG() << "Start";

    mStart = true;
    mCondVar.NotifyOne();

    return aos::ErrorEnum::eNone;
}

aos::Error ClockSync::Sync(const aos::Time& time)
{
    aos::LockGuard lock(mMutex);

    LOG_DBG() << "Sync: time = " << time;

    if (llabs(aos::Time::Now().Sub(time)) > cMaxTimeDiff) {
        LOG_DBG() << "Set time: time = " << time;

        auto ts = time.UnixTime();

        auto ret = clock_settime(CLOCK_REALTIME, &ts);
        if (ret != 0) {
            return AOS_ERROR_WRAP(ret);
        }
    }

    mSync = true;
    mCondVar.NotifyOne();

    return aos::ErrorEnum::eNone;
}

ClockSync::~ClockSync()
{
    {
        aos::LockGuard lock(mMutex);

        mClose = true;
        mCondVar.NotifyOne();
    }

    mThread.Join();
}
