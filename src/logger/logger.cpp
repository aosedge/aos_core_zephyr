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
        static void LogCallback(aos::LogLevel level, const aos::String& message)                                       \
        {                                                                                                              \
            switch (level.GetValue()) {                                                                                \
            case aos::LogLevelEnum::eDebug:                                                                            \
                LOG_DBG("%s", message.CStr());                                                                         \
                                                                                                                       \
                break;                                                                                                 \
                                                                                                                       \
            case aos::LogLevelEnum::eInfo:                                                                             \
                LOG_INF("%s", message.CStr());                                                                         \
                                                                                                                       \
                break;                                                                                                 \
                                                                                                                       \
            case aos::LogLevelEnum::eWarning:                                                                          \
                LOG_WRN("%s", message.CStr());                                                                         \
                                                                                                                       \
                break;                                                                                                 \
                                                                                                                       \
            case aos::LogLevelEnum::eError:                                                                            \
                LOG_ERR("%s", message.CStr());                                                                         \
                                                                                                                       \
                break;                                                                                                 \
                                                                                                                       \
            default:                                                                                                   \
                LOG_ERR("Unknown log level received: %s", level.ToString().CStr());                                    \
                                                                                                                       \
                break;                                                                                                 \
            }                                                                                                          \
        }                                                                                                              \
    }

// internal logs

LOG_CALLBACK(app);
LOG_CALLBACK(clocksync);
LOG_CALLBACK(communication);
LOG_CALLBACK(downloader);
LOG_CALLBACK(ocispec);
LOG_CALLBACK(provisioning);
LOG_CALLBACK(resourcemanager);
LOG_CALLBACK(runner);
LOG_CALLBACK(storage);

// Aos lib logs

LOG_CALLBACK(certhandler);
LOG_CALLBACK(crypto);
LOG_CALLBACK(launcher);
LOG_CALLBACK(monitoring);
LOG_CALLBACK(servicemanager);
LOG_CALLBACK(pkcs11);

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

void Logger::LogCallback(aos::LogModule module, aos::LogLevel level, const aos::String& message)
{
    switch (static_cast<int>(module.GetValue())) {
        // internal logs

    case static_cast<int>(Module::eApp):
        log_app::LogCallback(level, message);

        break;

    case static_cast<int>(Module::eCommunication):
        log_communication::LogCallback(level, message);

        break;

    case static_cast<int>(Module::eClockSync):
        log_clocksync::LogCallback(level, message);

        break;

    case static_cast<int>(Module::eDownloader):
        log_downloader::LogCallback(level, message);

        break;

    case static_cast<int>(Module::eOCISpec):
        log_ocispec::LogCallback(level, message);

        break;

    case static_cast<int>(Module::eProvisioning):
        log_provisioning::LogCallback(level, message);

        break;

    case static_cast<int>(Module::eResourceManager):
        log_resourcemanager::LogCallback(level, message);

        break;

    case static_cast<int>(Module::eRunner):
        log_runner::LogCallback(level, message);

        break;

    case static_cast<int>(Module::eStorage):
        log_storage::LogCallback(level, message);

        break;

        // Aos lib logs

    case static_cast<int>(aos::LogModuleEnum::eCommonPKCS11):
        log_pkcs11::LogCallback(level, message);

        break;

    case static_cast<int>(aos::LogModuleEnum::eCommonCrypto):
        log_crypto::LogCallback(level, message);

        break;

    case static_cast<int>(aos::LogModuleEnum::eIAMCertHandler):
        log_certhandler::LogCallback(level, message);

        break;

    case static_cast<int>(aos::LogModuleEnum::eSMLauncher):
        log_launcher::LogCallback(level, message);

        break;

    case static_cast<int>(aos::LogModuleEnum::eCommonMonitoring):
        log_monitoring::LogCallback(level, message);

        break;

    case static_cast<int>(aos::LogModuleEnum::eSMServiceManager):
        log_servicemanager::LogCallback(level, message);

        break;

    default:
        LOG_MODULE_WRN(static_cast<aos::LogModule::EnumType>(Logger::Module::eCommunication))
            << "Log from unknown module received: module = " << module << ", level = " << level
            << ", message = " << message;
    }
}
