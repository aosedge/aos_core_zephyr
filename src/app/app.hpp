/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef APP_HPP_
#define APP_HPP_

#include <aos/common/crypto/mbedtls/cryptoprovider.hpp>
#include <aos/common/cryptoutils.hpp>
#include <aos/common/monitoring/resourcemonitor.hpp>
#include <aos/common/tools/error.hpp>
#include <aos/common/tools/noncopyable.hpp>
#include <aos/iam/certhandler.hpp>
#include <aos/iam/certmodules/pkcs11/pkcs11.hpp>
#include <aos/iam/provisionmanager.hpp>
#include <aos/sm/launcher.hpp>
#include <aos/sm/servicemanager.hpp>

#include "clocksync/clocksync.hpp"
#include "communication/channelmanager.hpp"
#include "communication/xenvchan.hpp"
#include "downloader/downloader.hpp"
#include "monitoring/resourceusageprovider.hpp"
#include "nodeinfoprovider/nodeinfoprovider.hpp"
#include "ocispec/ocispec.hpp"
#include "provisionmanager/provisionmanagercallback.hpp"
#include "resourcemanager/resourcemanager.hpp"
#include "runner/runner.hpp"
#include "smclient/smclient.hpp"
#include "storage/storage.hpp"

namespace aos::zephyr::app {

/**
 * Aos zephyr application.
 */
class App : private NonCopyable {
public:
    /**
     * Initializes application.
     *
     * @return Error
     */
    Error Init();

    /**
     * Returns Aos application instance.
     * @return App&
     */
    static App& Get() { return sApp; };

private:
    static constexpr auto cPKCS11ModuleLibrary    = AOS_CONFIG_CRYPTOUTILS_DEFAULT_PKCS11_LIB;
    static constexpr auto cPKCS11ModuleTokenLabel = "aoscore";
    static constexpr auto cPKCS11ModulePinFile    = CONFIG_AOS_PKCS11_MODULE_PIN_FILE;
    static constexpr auto cNodeType               = CONFIG_AOS_NODE_TYPE;
    static constexpr auto cNodeConfigFile         = CONFIG_AOS_NODE_CONFIG_FILE;

    Error InitCommon();
    Error InitIAM();
    Error InitSM();
    Error InitZephyr();
    Error InitCommunication();

    static App sApp;

    aos::crypto::MbedTLSCryptoProvider mCryptoProvider;
    aos::cryptoutils::CertLoader       mCertLoader;
    aos::monitoring::ResourceMonitor   mResourceMonitor;
    aos::pkcs11::PKCS11Manager         mPKCS11Manager;

    iam::certhandler::CertHandler           mCertHandler;
    iam::certhandler::CertModule            mIAMCertModule;
    iam::certhandler::CertModule            mSMCertModule;
    iam::certhandler::PKCS11Module          mIAMHSMModule;
    iam::certhandler::PKCS11Module          mSMHSMModule;
    iam::provisionmanager::ProvisionManager mProvisionManager;

    sm::launcher::Launcher               mLauncher;
    sm::resourcemanager::ResourceManager mResourceManager;
    sm::servicemanager::ServiceManager   mServiceManager;

    clocksync::ClockSync                       mClockSync;
    communication::ChannelManager              mChannelManager;
    communication::XenVChan                    mTransport;
    downloader::Downloader                     mDownloader;
    monitoring::ResourceUsageProvider          mResourceUsageProvider;
    nodeinfoprovider::NodeInfoProvider         mNodeInfoProvider;
    ocispec::OCISpec                           mJsonOciSpec;
    provisionmanager::ProvisionManagerCallback mProvisionManagerCallback;
    resourcemanager::HostDeviceManager         mHostDeviceManager;
    resourcemanager::HostGroupManager          mHostGroupManager;
    resourcemanager::JSONProvider              mResourceManagerJSONProvider;
    runner::Runner                             mRunner;
    smclient::SMClient                         mSMClient;
    storage::Storage                           mStorage;
};

} // namespace aos::zephyr::app

#endif
