/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LOGGER_HPP_
#define LOGGER_HPP_

#if CONFIG_LOG_RUNTIME_FILTERING
#include <zephyr/logging/log_ctrl.h>
#endif

#include <aos/common/tools/log.hpp>
#include <aos/common/tools/map.hpp>

namespace aos::zephyr::logger {

/**
 * Logger instance.
 */
class Logger {
public:
    /**
     * Inits logging system.
     *
     * return Error.
     */
    static Error Init();

private:
    static constexpr auto cMaxLogModules = 32;
#if CONFIG_LOG_RUNTIME_FILTERING
    static constexpr auto cRuntimeLogLevel = CONFIG_AOS_CORE_RUNTIME_LOG_LEVEL;
#endif

    static void LogCallback(const String& module, LogLevel level, const String& message);
#if CONFIG_LOG_RUNTIME_FILTERING
    static Error SetLogLevel(const String& module, int level);
#endif

    static StaticMap<String, void (*)(LogLevel, const String&), cMaxLogModules> sLogCallbacks;
};

} // namespace aos::zephyr::logger

#endif
