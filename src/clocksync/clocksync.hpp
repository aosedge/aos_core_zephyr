/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef AOS_CLOCKSYNC_HPP_
#define AOS_CLOCKSYNC_HPP_

#include "aos/common/connectionsubsc.hpp"
#include "aos/common/tools/error.hpp"
#include <aos/common/tools/thread.hpp>

/**
 * Clock sync sender interface.
 */
class ClockSyncSenderItf {
public:
    /**
     * Destructor
     */
    virtual ~ClockSyncSenderItf() = default;

    /**
     * Send clock sync request.
     *
     * @return Error
     */
    virtual aos::Error SendClockSyncRequest() = 0;
};

/**
 * Resource monitor interface.
 */
class ClockSyncItf {
public:
    /**
     * Destructor
     */
    virtual ~ClockSyncItf() = default;

    /**
     * Process clock sync.
     *
     * @param ts timestamp.
     * @return Error
     */
    virtual aos::Error ProcessClockSync(struct timespec ts) = 0;
};

/**
 * Clock sync.
 */
class ClockSync : public ClockSyncItf, public aos::ConnectionSubscriberItf {
public:
    /**
     * Creates clock sync instance.
     */
    ClockSync() = default;

    /**
     * Destructor.
     */
    ~ClockSync();

    /**
     * Initializes clock sync instance.
     * @param sender sender.
     * @param connectionPublisher connection publisher.
     * @return aos::Error.
     */
    aos::Error Init(ClockSyncSenderItf& sender, aos::ConnectionPublisherItf& connectionPublisher);

    /**
     * Process clock sync.
     *
     * @param ts timestamp.
     * @return Error
     */
    aos::Error ProcessClockSync(struct timespec ts) override;

    /**
     * Respond to a connection event.
     */
    void OnConnect() override;

    /**
     * Respond to a disconnection event.
     */
    void OnDisconnect() override;

private:
    static constexpr auto cClockSyncPeriodSec = CONFIG_AOS_CLOCK_SYNC_PERIOD_SEC;

    aos::Error RunClockSync();

    ClockSyncSenderItf*          mSender {};
    aos::ConnectionPublisherItf* mConnectionPublisher {};
    aos::Thread<>                mThreadClockSync = {};
    aos::Mutex                   mMutex;
    bool                         mFinish {};
    bool                         mSendClockSyncRequest {};
    aos::ConditionalVariable     mClockSyncRequestCV {mMutex};
};

#endif
