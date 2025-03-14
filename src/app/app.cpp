/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "app.hpp"
#include "log.hpp"

namespace aos::zephyr::app {

/***********************************************************************************************************************
 * Variables
 **********************************************************************************************************************/

App App::sApp;

/***********************************************************************************************************************
 * Public
 **********************************************************************************************************************/

Error App::Init()
{
    LOG_INF() << "Initialize application";

    if (auto err = InitZephyr(); !err.IsNone()) {
        return err;
    }

    if (auto err = InitCommon(); !err.IsNone()) {
        return err;
    }

    if (auto err = InitIAM(); !err.IsNone()) {
        return err;
    }

    if (auto err = InitSM(); !err.IsNone()) {
        return err;
    }

    if (auto err = InitCommunication(); !err.IsNone()) {
        return err;
    }

    return ErrorEnum::eNone;
}

Error App::Start()
{
    LOG_INF() << "Start application";

    if (auto err = mLauncher.Start(); !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    if (auto err = mResourceMonitor.Start(); !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    if (auto err = mChannelManager.Start(); !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    if (auto err = mIAMClient.Start(); !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    if (auto err = mSMClient.Start(); !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    return ErrorEnum::eNone;
}

Error App::Stop()
{
    LOG_INF() << "Stop application";

    if (auto err = mLauncher.Stop(); !err.IsNone()) {
        LOG_ERR() << "Failed to stop launcher: err=" << err;
    }

    if (auto err = mResourceMonitor.Stop(); !err.IsNone()) {
        LOG_ERR() << "Failed to stop resource monitor: err=" << err;
    }

    if (auto err = mClockSync.Stop(); !err.IsNone()) {
        LOG_ERR() << "Failed to stop clock sync: err=" << err;
    }

    if (auto err = mChannelManager.Stop(); !err.IsNone()) {
        LOG_ERR() << "Failed to stop channel manager: err=" << err;
    }

    if (auto err = mIAMClient.Stop(); !err.IsNone()) {
        LOG_ERR() << "Failed to stop IAM client: err=" << err;
    }

    if (auto err = mSMClient.Stop(); !err.IsNone()) {
        LOG_ERR() << "Failed to stop SM client: err=" << err;
    }

    return ErrorEnum::eNone;
}

/***********************************************************************************************************************
 * Private
 **********************************************************************************************************************/

Error App::InitCommon()
{
    if (auto err = mCryptoProvider.Init(); !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    if (auto err = mCertLoader.Init(mCryptoProvider, mPKCS11Manager); !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    aos::monitoring::Config monitorConfig;

    if (auto err = mResourceMonitor.Init(monitorConfig, mNodeInfoProvider, mResourceManager, mResourceUsageProvider,
            mSMClient, mSMClient, mSMClient);
        !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    return ErrorEnum::eNone;
}

Error App::InitIAM()
{
#ifdef CONFIG_NATIVE_APPLICATION
    if (auto ret = setenv("SOFTHSM2_CONF", "softhsm/softhsm2.conf", true); ret != 0) {
        return AOS_ERROR_WRAP(Error(ret, "can't set softhsm env"));
    }

    if (auto err = FS::MakeDirAll(cHSMDir); !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }
#endif

    iam::certhandler::ExtendedKeyUsage keyUsage[] = {iam::certhandler::ExtendedKeyUsageEnum::eClientAuth};

    // Register iam cert module

    if (auto err
        = mIAMHSMModule.Init("iam", {cPKCS11ModuleLibrary, {}, {}, cPKCS11ModuleTokenLabel, cPKCS11ModulePinFile},
            mPKCS11Manager, mCryptoProvider);
        !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    if (auto err = mIAMCertModule.Init("iam",
            {crypto::KeyTypeEnum::eECDSA, 2, Array<iam::certhandler::ExtendedKeyUsage>(keyUsage, ArraySize(keyUsage))},
            mCryptoProvider, mIAMHSMModule, mStorage);
        !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    if (auto err = mCertHandler.RegisterModule(mIAMCertModule); !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    // Register sm cert module

    if (auto err
        = mSMHSMModule.Init("sm", {cPKCS11ModuleLibrary, {}, {}, cPKCS11ModuleTokenLabel, cPKCS11ModulePinFile},
            mPKCS11Manager, mCryptoProvider);
        !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    if (auto err = mSMCertModule.Init("sm",
            {crypto::KeyTypeEnum::eECDSA, 2, Array<iam::certhandler::ExtendedKeyUsage>(keyUsage, ArraySize(keyUsage))},
            mCryptoProvider, mSMHSMModule, mStorage);
        !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    if (auto err = mCertHandler.RegisterModule(mSMCertModule); !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    if (auto err = mProvisionManager.Init(mProvisionManagerCallback, mCertHandler); !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    return ErrorEnum::eNone;
}

Error App::InitSM()
{
    aos::sm::launcher::Config launcherConfig;

    if (auto err = mLauncher.Init(launcherConfig, mNodeInfoProvider, mServiceManager, mLayerManager, mResourceManager,
            mNetworkManager, mPermHandler, mRunner, mRuntime, mResourceMonitor, mJsonOciSpec, mSMClient, mSMClient,
            mStorage);
        !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    if (auto err = mResourceManager.Init(mResourceManagerJSONProvider, mHostDeviceManager, cNodeType, cNodeConfigFile);
        !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    aos::sm::servicemanager::Config smConfig;
    smConfig.mServicesDir = CONFIG_AOS_SERVICES_DIR;
    smConfig.mDownloadDir = CONFIG_AOS_DOWNLOAD_DIR;

    if (auto err = mServiceManager.Init(smConfig, mJsonOciSpec, mDownloader, mStorage, mServiceSpaceAllocator,
            mDownloadSpaceAllocator, mImageHandler);
        !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    sm::layermanager::Config layerConfig;
    layerConfig.mLayersDir   = CONFIG_AOS_LAYERS_DIR;
    layerConfig.mDownloadDir = CONFIG_AOS_DOWNLOAD_DIR;
    layerConfig.mTTL         = Time::cHours * 24;

    if (auto err = mLayerManager.Init(
            layerConfig, mLayerSpaceAllocator, mDownloadSpaceAllocator, mStorage, mDownloader, mImageHandler);
        !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    return ErrorEnum::eNone;
}

Error App::InitZephyr()
{
#ifdef CONFIG_NATIVE_APPLICATION
    if (auto err = FS::MakeDirAll(cAosDiskMountPoint); !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }
#endif

    if (auto err = mStorage.Init(); !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    if (auto err = mNodeInfoProvider.Init(); !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    if (auto err = mClockSync.Init(mSMClient); !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    if (auto err = mDownloader.Init(mSMClient); !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    if (auto err = mResourceUsageProvider.Init(); !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    if (auto err = mRunner.Init(mLauncher); !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    return ErrorEnum::eNone;
}

Error App::InitCommunication()
{
#ifdef CONFIG_NATIVE_APPLICATION
    if (auto err = mTransport.Init(communication::Socket::cServerAddress, communication::Socket::cServerPort);
        !err.IsNone()) {
        return err;
    }
#else
    if (auto err = mTransport.Init(communication::XenVChan::cReadPath, communication::XenVChan::cWritePath);
        !err.IsNone()) {
        return err;
    }
#endif

    if (auto err = mChannelManager.Init(mTransport); !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    if (auto err
        = mIAMClient.Init(mClockSync, mNodeInfoProvider, mProvisionManager, mChannelManager, mCertHandler, mCertLoader);
        !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    if (auto err = mSMClient.Init(mNodeInfoProvider, mLauncher, mResourceManager, mResourceMonitor, mDownloader,
            mClockSync, mChannelManager, mCertHandler, mCertLoader);
        !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    return ErrorEnum::eNone;
}

} // namespace aos::zephyr::app
