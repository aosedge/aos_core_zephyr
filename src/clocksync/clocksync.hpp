/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef AOS_CLOCKSYNC_HPP_
#define AOS_CLOCKSYNC_HPP_

#include <aos/common/tools/thread.hpp>

namespace aos::zephyr::clocksync {

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
};

class ClockSyncSubscriberItf {
public:
    /**
     * Destructor.
     */
    virtual ~ClockSyncSubscriberItf() = default;

    /**
     * Notifies subscriber clock is synced.
     */
    virtual void OnClockSynced() = 0;

    /**
     * Notifies subscriber clock is unsynced.
     */
    virtual void OnClockUnsynced() = 0;
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
    virtual Error Start() = 0;

    /**
     * Synchronizes system clock.
     *
     * @param time current time to set.
     * @return aos::Error
     */
    virtual Error Sync(const aos::Time& time) = 0;

    /**
     * Subscribes for clock sync notifications.
     *
     * @param subscriber clock sync notification subscriber.
     */
    virtual Error Subscribe(ClockSyncSubscriberItf& subscriber) = 0;

    /**
     * Unsubscribes from clock sync notifications.
     *
     * @param subscriber clock sync notification subscriber.
     */
    virtual void Unsubscribe(ClockSyncSubscriberItf& subscriber) = 0;
};

/**
 * Clock sync instance.
 */
class ClockSync : public ClockSyncItf {
public:
    /**
     * Initializes clock sync instance.
     *
     * @param sender sender.
     * @return Error.
     */
    Error Init(ClockSyncSenderItf& sender);

    /**
     * Starts clock sync.
     *
     * @return Error.
     */
    Error Start() override;

    /**
     * Stops clock sync.
     *
     * @return Error.
     */
    Error Stop();

    /**
     * Synchronizes system clock.
     *
     * @param time current time to set.
     * @return Error
     */
    Error Sync(const Time& time) override;

    /**
     * Subscribes for clock sync notifications.
     *
     * @param subscriber clock sync notification subscriber.
     * @return Error.
     */
    Error Subscribe(ClockSyncSubscriberItf& subscriber) override;

    /**
     * Unsubscribes from clock sync notifications.
     *
     * @param subscriber clock sync notification subscriber.
     */
    void Unsubscribe(ClockSyncSubscriberItf& subscriber) override;

private:
    static constexpr auto cSendPeriod     = CONFIG_AOS_CLOCK_SYNC_SEND_PERIOD_SEC * Time::cSeconds;
    static constexpr auto cSyncTimeout    = CONFIG_AOS_CLOCK_SYNC_TIMEOUT_SEC * Time::cSeconds;
    static constexpr auto cMaxTimeDiff    = CONFIG_AOS_CLOCK_SYNC_MAX_DIFF_MSEC * Time::cMilliseconds;
    static constexpr auto cMaxSubscribers = 2;

    void ClockSyncNotification();

    ClockSyncSenderItf* mSender {};

    Thread<>            mThread;
    Mutex               mMutex;
    ConditionalVariable mCondVar;

    Time mSyncTime;
    bool mSync    = false;
    bool mSynced  = false;
    bool mStart   = false;
    bool mStarted = false;
    bool mClose   = false;

    StaticArray<ClockSyncSubscriberItf*, cMaxSubscribers> mConnectionSubscribers;
};

} // namespace aos::zephyr::clocksync

#endif
