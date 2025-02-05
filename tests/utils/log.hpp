/*
 * Copyright (C) 2024 Renesas Electronics Corporation.
 * Copyright (C) 2024 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _UTILS_LOG_HPP
#define _UTILS_LOG_HPP

#include <aos/common/tools/log.hpp>

void TestLogCallback(const aos::String& module, aos::LogLevel level, const aos::String& message);

#endif
