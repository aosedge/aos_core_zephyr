/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef AOS_CLOCKSYNC_HPP_
#define AOS_CLOCKSYNC_HPP_

#include <aos/common/tools/time.hpp>

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

#endif
