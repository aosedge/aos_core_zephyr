/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>

#include "logger.hpp"

/***********************************************************************************************************************
 * Consts
 **********************************************************************************************************************/

static const aos::String cLogModuleApp              = "app";
static const aos::String cLogModuleCommunication    = "communication";
static const aos::String cLogModuleClockSync        = "clocksync";
static const aos::String cLogModuleDownloader       = "downloader";
static const aos::String cLogModuleOCISpec          = "ocispec";
static const aos::String cLogModuleProvisioning     = "provisioning";
static const aos::String cLogModuleResourceManager  = "resourcemanager";
static const aos::String cLogModuleRunner           = "runner";
static const aos::String cLogModuleStorage          = "storage";
static const aos::String cLogModuleNodeInfoProvider = "nodeinfoprovider";

static const aos::String cLogModuleCerthandler    = "certhandler";
static const aos::String cLogModuleCrypto         = "crypto";
static const aos::String cLogModuleLauncher       = "launcher";
static const aos::String cLogModuleMonitoring     = "monitoring";
static const aos::String cLogModuleServiceManager = "servicemanager";
static const aos::String cLogModulePKCS11         = "pkcs11";

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
LOG_CALLBACK(nodeinfoprovider);

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

void Logger::LogCallback(const char* module, aos::LogLevel level, const aos::String& message)
{
    aos::String logModule(module);

    if (logModule == cLogModuleApp) {
        log_app::LogCallback(level, message);
    } else if (logModule == cLogModuleCommunication) {
        log_communication::LogCallback(level, message);
    } else if (logModule == cLogModuleClockSync) {
        log_clocksync::LogCallback(level, message);
    } else if (logModule == cLogModuleDownloader) {
        log_downloader::LogCallback(level, message);
    } else if (logModule == cLogModuleOCISpec) {
        log_ocispec::LogCallback(level, message);
    } else if (logModule == cLogModuleProvisioning) {
        log_provisioning::LogCallback(level, message);
    } else if (logModule == cLogModuleResourceManager) {
        log_resourcemanager::LogCallback(level, message);
    } else if (logModule == cLogModuleRunner) {
        log_runner::LogCallback(level, message);
    } else if (logModule == cLogModuleStorage) {
        log_storage::LogCallback(level, message);
    } else if (logModule == cLogModuleNodeInfoProvider) {
        log_nodeinfoprovider::LogCallback(level, message);
    } else if (logModule == cLogModuleCerthandler) {
        log_certhandler::LogCallback(level, message);
    } else if (logModule == cLogModuleCrypto) {
        log_crypto::LogCallback(level, message);
    } else if (logModule == cLogModuleLauncher) {
        log_launcher::LogCallback(level, message);
    } else if (logModule == cLogModuleMonitoring) {
        log_monitoring::LogCallback(level, message);
    } else if (logModule == cLogModuleServiceManager) {
        log_servicemanager::LogCallback(level, message);
    } else if (logModule == cLogModulePKCS11) {
        log_pkcs11::LogCallback(level, message);
    } else {
        LOG_MODULE_WRN(cLogModuleCommunication.CStr())
            << "Log from unknown module received: module=" << module << ", level=" << level << ", message=" << message;
    }
}
