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

    if (auto err = mResourceMonitor.Init(mNodeInfoProvider, mResourceUsageProvider, mSMClient, mSMClient);
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
    if (auto err
        = mLauncher.Init(mServiceManager, mRunner, mJsonOciSpec, mSMClient, mStorage, mResourceMonitor, mSMClient);
        !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    if (auto err = mResourceManager.Init(
            mResourceManagerJSONProvider, mHostDeviceManager, mHostGroupManager, cNodeType, cNodeConfigFile);
        !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    if (auto err = mServiceManager.Init(mJsonOciSpec, mDownloader, mStorage); !err.IsNone()) {
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

    if (auto err = mClockSync.Init(mSMClient); !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    if (auto err = mIAMClient.Init(mClockSync, mNodeInfoProvider, mChannelManager); !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    if (auto err = mDownloader.Init(mSMClient); !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    if (auto err = mResourceUsageProvider.Init(mNodeInfoProvider); !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    if (auto err = mNodeInfoProvider.Init(); !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    if (auto err = mRunner.Init(mLauncher); !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    if (auto err = mSMClient.Init(mClockSync, mChannelManager); !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    if (auto err = mStorage.Init(); !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    if (auto err = InitCommunication(); !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    return ErrorEnum::eNone;
}

Error App::InitCommunication()
{
    if (auto err = mTransport.Init(communication::XenVChan::cXSReadPath, communication::XenVChan::cXSWritePath);
        !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    if (auto err = mChannelManager.Init(mTransport); !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    return ErrorEnum::eNone;
}

} // namespace aos::zephyr::app
