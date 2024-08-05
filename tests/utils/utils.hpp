/*
 * Copyright (C) 2024 Renesas Electronics Corporation.
 * Copyright (C) 2024 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _TEST_UTILS_HPP
#define _TEST_UTILS_HPP

#include <aos/common/tools/error.hpp>

/**
 * Converts Aos error to string.
 *
 * @param err aos error.
 * @return const char*
 */
const char* AosErrorToStr(const aos::Error& err);

#endif
