/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>

#include "logger.hpp"

namespace aos::zephyr::logger {

/***********************************************************************************************************************
 * Consts
 **********************************************************************************************************************/

static const String cLogModuleApp              = "app";
static const String cLogModuleClockSync        = "clocksync";
static const String cLogModuleCommunication    = "communication";
static const String cLogModuleDownloader       = "downloader";
static const String cLogModuleIAMClient        = "iamclient";
static const String cLogModuleNodeInfoProvider = "nodeinfoprovider";
static const String cLogModuleOCISpec          = "ocispec";
static const String cLogModuleProvisionManager = "provisionmanager";
static const String cLogModuleResourceManager  = "resourcemanager";
static const String cLogModuleRunner           = "runner";
static const String cLogModuleSMClient         = "smclient";
static const String cLogModuleStorage          = "storage";

static const String cLogModuleCerthandler    = "certhandler";
static const String cLogModuleCrypto         = "crypto";
static const String cLogModuleLauncher       = "launcher";
static const String cLogModuleMonitoring     = "monitoring";
static const String cLogModulePKCS11         = "pkcs11";
static const String cLogModuleServiceManager = "servicemanager";

/***********************************************************************************************************************
 * Log module callbacks
 **********************************************************************************************************************/

#define LOG_CALLBACK(name)                                                                                             \
    namespace log_##name                                                                                               \
    {                                                                                                                  \
        LOG_MODULE_REGISTER(name, CONFIG_LOG_DEFAULT_LEVEL);                                                           \
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

void Logger::Init()
{
    Log::SetCallback(LogCallback);
}

/***********************************************************************************************************************
 * Public
 **********************************************************************************************************************/

void Logger::LogCallback(const char* module, LogLevel level, const String& message)
{
    String logModule(module);

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
    } else if (logModule == cLogModuleProvisionManager) {
        log_provisionmanager::LogCallback(level, message);
    } else if (logModule == cLogModuleResourceManager) {
        log_resourcemanager::LogCallback(level, message);
    } else if (logModule == cLogModuleRunner) {
        log_runner::LogCallback(level, message);
    } else if (logModule == cLogModuleSMClient) {
        log_smclient::LogCallback(level, message);
    } else if (logModule == cLogModuleIAMClient) {
        log_iamclient::LogCallback(level, message);
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

} // namespace aos::zephyr::logger
