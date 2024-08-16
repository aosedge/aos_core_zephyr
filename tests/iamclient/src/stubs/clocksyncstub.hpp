/*
 * Copyright (C) 2024 Renesas Electronics Corporation.
 * Copyright (C) 2024 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CLOCKSYNCSTUB_HPP_
#define CLOCKSYNCSTUB_HPP_

#include "clocksync/clocksync.hpp"

class ClockSyncStub : public aos::zephyr::clocksync::ClockSyncItf {
public:
    aos::Error Start() override { return aos::ErrorEnum::eNone; }

    aos::Error Sync(const aos::Time& time) override { return aos::ErrorEnum::eNone; }

    aos::Error Subscribe(aos::zephyr::clocksync::ClockSyncSubscriberItf& subscriber) override
    {
        return aos::ErrorEnum::eNone;
    }

    void Unsubscribe(aos::zephyr::clocksync::ClockSyncSubscriberItf& subscriber) override {};
};

#endif
