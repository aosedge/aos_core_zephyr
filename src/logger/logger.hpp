/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LOGGER_HPP_
#define LOGGER_HPP_

#include <aos/common/tools/log.hpp>

namespace aos::zephyr::logger {

/**
 * Logger instance.
 */
class Logger {
public:
    /**
     * Inits logging system.
     */
    static void Init();

private:
    static void LogCallback(const char* module, LogLevel level, const String& message);
};

} // namespace aos::zephyr::logger

#endif
