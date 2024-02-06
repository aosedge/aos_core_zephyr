/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef APP_HPP_
#define APP_HPP_

#include <aos/common/crypto/mbedtls/cryptoprovider.hpp>
#include <aos/common/monitoring.hpp>
#include <aos/common/tools/error.hpp>
#include <aos/common/tools/noncopyable.hpp>
#include <aos/iam/certmodules/pkcs11/pkcs11.hpp>
#include <aos/sm/launcher.hpp>
#include <aos/sm/servicemanager.hpp>

#include "clocksync/clocksync.hpp"
#include "communication/communication.hpp"
#include "communication/vchannel.hpp"
#include "monitoring/resourceusageprovider.hpp"
#include "ocispec/ocispec.hpp"
#include "provisioning/provisioning.hpp"
#include "runner/runner.hpp"
#include "storage/storage.hpp"

/**
 * Aos zephyr application.
 */
class App : private aos::NonCopyable {
public:
    /**
     * Initializes application.
     *
     * @return aos::Error
     */
    aos::Error Init();

    /**
     * Returns Aos application instance.
     * @return App&
     */
    static App& Get() { return sApp; };

private:
    static constexpr auto cPKCS11ModuleLibrary    = AOS_CONFIG_CRYPTOUTILS_DEFAULT_PKCS11_LIB;
    static constexpr auto cPKCS11ModuleTokenLabel = "aoscore";
    static constexpr auto cPKCS11ModulePinFile    = CONFIG_AOS_PKCS11_MODULE_PIN_FILE;

    aos::Error InitCertHandler();

    static App                              sApp;
    aos::monitoring::ResourceMonitor        mResourceMonitor;
    aos::sm::launcher::Launcher             mLauncher;
    aos::sm::servicemanager::ServiceManager mServiceManager;
    aos::iam::certhandler::CertModule       mIAMCertModule;
    aos::iam::certhandler::PKCS11Module     mIAMHSMModule;
    aos::iam::certhandler::CertModule       mSMCertModule;
    aos::iam::certhandler::PKCS11Module     mSMHSMModule;
    aos::iam::certhandler::CertHandler      mCertHandler;
    aos::crypto::MbedTLSCryptoProvider      mCryptoProvider;
    aos::pkcs11::PKCS11Manager              mPKCS11Manager;
    ClockSync                               mClockSync;
    Communication                           mCommunication;
    VChannel                                mCommOpenChannel;
    VChannel                                mCommSecureChannel;
    Downloader                              mDownloader;
    OCISpec                                 mJsonOciSpec;
    ResourceManager                         mResourceManager;
    ResourceUsageProvider                   mResourceUsageProvider;
    Runner                                  mRunner;
    Storage                                 mStorage;
    Provisioning                            mProvisioning;
};

#endif
