/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef AOS_CLOCKSYNC_HPP_
#define AOS_CLOCKSYNC_HPP_

#include <aos/common/tools/thread.hpp>

/**
 * Clock sync sender interface.
 */
class ClockSyncSenderItf {
public:
    /**
     * Destructor.
     */
    virtual ~ClockSyncSenderItf() = default;

    /**
     * Sends clock sync request.
     *
     * @return aos::Error.
     */
    virtual aos::Error SendClockSyncRequest() = 0;

    /**
     * Notifies sender that clock is synced.
     */
    virtual void ClockSynced() = 0;

    /**
     * Notifies sender that clock is unsynced.
     */
    virtual void ClockUnsynced() = 0;
};

/**
 * Clock sync interface.
 */
class ClockSyncItf {
public:
    /**
     * Destructor.
     */
    virtual ~ClockSyncItf() = default;

    /**
     * Starts clock sync.
     *
     * @return aos::Error.
     */
    virtual aos::Error Start() = 0;

    /**
     * Synchronizes system clock.
     *
     * @param time current time to set.
     * @return aos::Error
     */
    virtual aos::Error Sync(const aos::Time& time) = 0;
};

/**
 * Clock sync instance.
 */
class ClockSync : public ClockSyncItf {
public:
    /**
     * Destructor.
     */
    ~ClockSync();

    /**
     * Initializes clock sync instance.
     * @param sender sender.
     * @return aos::Error.
     */
    aos::Error Init(ClockSyncSenderItf& sender);

    /**
     * Starts clock sync.
     *
     * @return aos::Error.
     */
    aos::Error Start() override;

    /**
     * Synchronizes system clock.
     *
     * @param time current time to set.
     * @return aos::Error
     */
    aos::Error Sync(const aos::Time& time) override;

private:
    static constexpr auto cSendPeriod  = CONFIG_AOS_CLOCK_SYNC_SEND_PERIOD_SEC * aos::Time::Seconds;
    static constexpr auto cSyncTimeout = CONFIG_AOS_CLOCK_SYNC_TIMEOUT_SEC * aos::Time::Seconds;
    static constexpr auto cMaxTimeDiff = CONFIG_AOS_CLOCK_SYNC_MAX_DIFF_MSEC * aos::Time::Milliseconds;

    ClockSyncSenderItf* mSender {};

    aos::Thread<>            mThread;
    aos::Mutex               mMutex;
    aos::ConditionalVariable mCondVar;

    aos::Time mSyncTime;
    bool      mSynced  = false;
    bool      mStart   = false;
    bool      mStarted = false;
    bool      mClose   = false;
};

#endif
