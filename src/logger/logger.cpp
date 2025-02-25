/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>

#if CONFIG_NATIVE_APPLICATION
#include <aos/common/tools/thread.hpp>
#endif

#include "logger.hpp"

namespace aos::zephyr::logger {

/***********************************************************************************************************************
 * Static
 **********************************************************************************************************************/

StaticMap<String, void (*)(LogLevel, const String&), Logger::cMaxLogModules> Logger::sLogCallbacks;

/***********************************************************************************************************************
 * Log module callbacks
 **********************************************************************************************************************/

#define LOG_CALLBACK(name)                                                                                             \
    namespace log_##name                                                                                               \
    {                                                                                                                  \
        LOG_MODULE_REGISTER(name, CONFIG_AOS_CORE_LOG_LEVEL);                                                          \
                                                                                                                       \
        static void LogCallback(LogLevel level, const String& message)                                                 \
        {                                                                                                              \
            switch (level.GetValue()) {                                                                                \
            case LogLevelEnum::eDebug:                                                                                 \
                LOG_DBG("%s", message.CStr());                                                                         \
                                                                                                                       \
                break;                                                                                                 \
                                                                                                                       \
            case LogLevelEnum::eInfo:                                                                                  \
                LOG_INF("%s", message.CStr());                                                                         \
                                                                                                                       \
                break;                                                                                                 \
                                                                                                                       \
            case LogLevelEnum::eWarning:                                                                               \
                LOG_WRN("%s", message.CStr());                                                                         \
                                                                                                                       \
                break;                                                                                                 \
                                                                                                                       \
            case LogLevelEnum::eError:                                                                                 \
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
LOG_CALLBACK(iamclient);
LOG_CALLBACK(nodeinfoprovider);
LOG_CALLBACK(ocispec);
LOG_CALLBACK(provisionmanager);
LOG_CALLBACK(resourcemanager);
LOG_CALLBACK(runner);
LOG_CALLBACK(smclient);
LOG_CALLBACK(storage);

// Aos lib logs
LOG_CALLBACK(certhandler);
LOG_CALLBACK(crypto);
LOG_CALLBACK(launcher);
LOG_CALLBACK(monitoring);
LOG_CALLBACK(pkcs11);
LOG_CALLBACK(servicemanager);

/***********************************************************************************************************************
 * Public
 **********************************************************************************************************************/

Error Logger::Init()
{
    Log::SetCallback(LogCallback);

    sLogCallbacks.Set("app", &log_app::LogCallback);
    sLogCallbacks.Set("certhandler", &log_certhandler::LogCallback);
    sLogCallbacks.Set("clocksync", &log_clocksync::LogCallback);
    sLogCallbacks.Set("communication", &log_communication::LogCallback);
    sLogCallbacks.Set("crypto", &log_crypto::LogCallback);
    sLogCallbacks.Set("downloader", &log_downloader::LogCallback);
    sLogCallbacks.Set("iamclient", &log_iamclient::LogCallback);
    sLogCallbacks.Set("launcher", &log_launcher::LogCallback);
    sLogCallbacks.Set("monitoring", &log_monitoring::LogCallback);
    sLogCallbacks.Set("nodeinfoprovider", &log_nodeinfoprovider::LogCallback);
    sLogCallbacks.Set("ocispec", &log_ocispec::LogCallback);
    sLogCallbacks.Set("pkcs11", &log_pkcs11::LogCallback);
    sLogCallbacks.Set("provisionmanager", &log_provisionmanager::LogCallback);
    sLogCallbacks.Set("resourcemanager", &log_resourcemanager::LogCallback);
    sLogCallbacks.Set("runner", &log_runner::LogCallback);
    sLogCallbacks.Set("servicemanager", &log_servicemanager::LogCallback);
    sLogCallbacks.Set("smclient", &log_smclient::LogCallback);
    sLogCallbacks.Set("storage", &log_storage::LogCallback);

#if CONFIG_LOG_RUNTIME_FILTERING
    for (auto& [module, _] : sLogCallbacks) {
        if (auto err = SetLogLevel(module, cRuntimeLogLevel); !err.IsNone()) {
            return err;
        }
    }
#endif

    return ErrorEnum::eNone;
}

/***********************************************************************************************************************
 * Private
 **********************************************************************************************************************/

void Logger::LogCallback(const String& module, LogLevel level, const String& message)
{
#ifdef CONFIG_NATIVE_APPLICATION
    static Mutex sMutex;

    LockGuard lock(sMutex);
#endif

    auto callbackIt = sLogCallbacks.Find(module);
    if (callbackIt == sLogCallbacks.end()) {
        LOG_MODULE_WRN("app") << "Log from unknown module received: module=" << module << ", level=" << level
                              << ", message=" << message;
        return;
    }

    callbackIt->mSecond(level, message);
}

#if CONFIG_LOG_RUNTIME_FILTERING
Error Logger::SetLogLevel(const String& module, int level)
{
    auto sourceID = log_source_id_get(module.CStr());
    if (sourceID < 0) {
        return AOS_ERROR_WRAP(ErrorEnum::eNotFound);
    }

    for (int i = 0; i < log_backend_count_get(); i++) {
        auto backend = log_backend_get(i);

        if (!log_backend_is_active(backend)) {
            continue;
        }

        if (auto ret = log_filter_set(backend, Z_LOG_LOCAL_DOMAIN_ID, sourceID, level); ret < 0) {
            return AOS_ERROR_WRAP(ret);
        }
    }

    return ErrorEnum::eNone;
}
#endif

} // namespace aos::zephyr::logger
