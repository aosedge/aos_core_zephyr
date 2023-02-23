/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>

#include "cmclient/cmclient.hpp"

ZTEST_SUITE(cmclient, NULL, NULL, NULL, NULL, NULL);

ZTEST(cmclient, test_init)
{
    CMClient client;

    auto err = client.Init();
    zassert_true(err.IsNone(), "init error: %s", err.ToString());
}
