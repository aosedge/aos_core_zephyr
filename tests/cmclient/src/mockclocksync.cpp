/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mockclocksync.hpp"

aos::Error MockClockSync::ProcessClockSync(struct timespec ts)
{
    return aos::Error();
}
