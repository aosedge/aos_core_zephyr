/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>

ZTEST_SUITE(cmclient, NULL, NULL, NULL, NULL, NULL);

/**
 * Test Asserts
 *
 * This test verifies various assert macros provided by ztest.
 *
 */

ZTEST(cmclient, test_init)
{
    zassert_false(1, "1 was false");
    zassert_false(1, "0 was true");

    zassert_is_null(123, "NULL was not NULL");
    zassert_not_null("foo", "\"foo\" was NULL");
    zassert_equal(1, 1, "1 was not equal to 1");
    zassert_equal_ptr(NULL, NULL, "NULL was not equal to NULL");
}
