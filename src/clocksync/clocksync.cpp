/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <unistd.h>
#include <zephyr/posix/time.h>

#include "clocksync.hpp"
#include "log.hpp"

aos::Error ClockSync::Init(ClockSyncSenderItf& sender, aos::ConnectionPublisherItf& connectionPublisher)
{
    LOG_DBG() << "Init clock sync";

    mSender = &sender;
    mConnectionPublisher = &connectionPublisher;

    mConnectionPublisher->Subscribes(this);

    return RunClockSync();
}

void ClockSync::OnConnect()
{
    aos::LockGuard lock(mMutex);

    LOG_DBG() << "Connection event";

    mSendClockSyncRequest = true;

    mClockSyncRequestCV.NotifyOne();
}

void ClockSync::OnDisconnect()
{
    aos::LockGuard lock(mMutex);

    LOG_DBG() << "Disconnection event";

    mSendClockSyncRequest = false;
}

aos::Error ClockSync::RunClockSync()
{
    return mThreadClockSync.Run([this](void*) {
        while (true) {
            mClockSyncRequestCV.Wait([this] { return mSendClockSyncRequest || mFinish; });

            if (mFinish) {
                break;
            }

            auto ret = mSender->SendClockSyncRequest();
            if (!ret.IsNone()) {
                LOG_ERR() << "Failed to send clock sync request: " << ret;
            }

            sleep(cClockSyncPeriodSec);
        }
    });
}

aos::Error ClockSync::ProcessClockSync(struct timespec ts)
{
    return clock_settime(CLOCK_REALTIME, &ts);
}

ClockSync::~ClockSync()
{
    mConnectionPublisher->Unsubscribes(this);

    {
        aos::LockGuard lock(mMutex);

        mFinish = true;

        mClockSyncRequestCV.NotifyOne();
    }

    mThreadClockSync.Join();
}
