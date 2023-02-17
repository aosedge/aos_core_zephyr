/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>

#include "logger.hpp"

/***********************************************************************************************************************
 * Log module callbacks
 **********************************************************************************************************************/

#define LOG_CALLBACK(name)                                                                                             \
    namespace log_##name                                                                                               \
    {                                                                                                                  \
        LOG_MODULE_REGISTER(name, CONFIG_LOG_DEFAULT_LEVEL);                                                           \
                                                                                                                       \
        static void LogCallback(aos::LogLevel level, const char* message)                                              \
        {                                                                                                              \
            switch (level.GetValue()) {                                                                                \
            case aos::LogLevelEnum::eDebug:                                                                            \
                LOG_DBG("%s", message);                                                                                \
                                                                                                                       \
                break;                                                                                                 \
                                                                                                                       \
            case aos::LogLevelEnum::eInfo:                                                                             \
                LOG_INF("%s", message);                                                                                \
                                                                                                                       \
                break;                                                                                                 \
                                                                                                                       \
            case aos::LogLevelEnum::eWarning:                                                                          \
                LOG_WRN("%s", message);                                                                                \
                                                                                                                       \
                break;                                                                                                 \
                                                                                                                       \
            case aos::LogLevelEnum::eError:                                                                            \
                LOG_ERR("%s", message);                                                                                \
                                                                                                                       \
                break;                                                                                                 \
                                                                                                                       \
            default:                                                                                                   \
                LOG_ERR("Unknown log level received");                                                                 \
                                                                                                                       \
                break;                                                                                                 \
            }                                                                                                          \
        }                                                                                                              \
    }

LOG_CALLBACK(app);
LOG_CALLBACK(launcher);

/***********************************************************************************************************************
 * Public
 **********************************************************************************************************************/

void Logger::Init()
{
    aos::Log::SetCallback(LogCallback);
}

/***********************************************************************************************************************
 * Public
 **********************************************************************************************************************/

void Logger::LogCallback(aos::LogModule module, aos::LogLevel level, const char* message)
{
    switch (static_cast<int>(module.GetValue())) {
    case static_cast<int>(Module::eApp):
        log_app::LogCallback(level, message);

        break;

    case static_cast<int>(aos::LogModuleEnum::eSMLauncher):
        log_launcher::LogCallback(level, message);

        break;

    default:
        break;
    }
}
