/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MOCK_CLOCKSYNC_HPP_
#define MOCK_CLOCKSYNC_HPP_

#include "clocksync/clocksync.hpp"

class MockClockSync : public ClockSyncItf {
public:
    aos::Error ProcessClockSync(struct timespec ts) override;
};

#endif
