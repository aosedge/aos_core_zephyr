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

aos::Error App::Init()
{
    LOG_INF() << "Initialize application";

    aos::Error err;

    if (!(err = mStorage.Init()).IsNone()) {
        return err;
    }

    if (!(err = mRunner.Init(mLauncher)).IsNone()) {
        return err;
    }

    if (!(err = mServiceManager.Init(mJsonOciSpec, mDownloader, mStorage)).IsNone()) {
        return err;
    }

    if (!(err = mResourceUsageProvider.Init()).IsNone()) {
        return err;
    }

    if (!(err = mResourceMonitor.Init(mResourceUsageProvider, mSMClient, mSMClient)).IsNone()) {
        return err;
    }

    if (!(err
            = mLauncher.Init(mServiceManager, mRunner, mJsonOciSpec, mSMClient, mStorage, mResourceMonitor, mSMClient))
             .IsNone()) {
        return err;
    }

    if (!(err = mClockSync.Init(mSMClient)).IsNone()) {
        return err;
    }

    if (!(err = mProvisioning.Init()).IsNone()) {
        return err;
    }

    if (!(err = mCryptoProvider.Init()).IsNone()) {
        return err;
    }

    if (!(err = mCertLoader.Init(mCryptoProvider, mPKCS11Manager)).IsNone()) {
        return err;
    }

    if (!(err = mResourceManager.Init(
              mResourceManagerJSONProvider, mHostDeviceManager, mHostGroupManager, cNodeType, cUnitConfigFile))
             .IsNone()) {
        return err;
    }

    if (!(err = InitCertHandler()).IsNone()) {
        return err;
    }

    if (!(err = InitCommunication()).IsNone()) {
        return err;
    }

    return aos::ErrorEnum::eNone;
}

/***********************************************************************************************************************
 * Private
 **********************************************************************************************************************/

aos::Error App::InitCertHandler()
{
    aos::Error                              err;
    aos::iam::certhandler::ExtendedKeyUsage keyUsage[] = {aos::iam::certhandler::ExtendedKeyUsageEnum::eClientAuth};

    // Register iam cert module

    if (!(err = mIAMHSMModule.Init("iam", {cPKCS11ModuleLibrary, {}, {}, cPKCS11ModuleTokenLabel, cPKCS11ModulePinFile},
              mPKCS11Manager, mCryptoProvider))
             .IsNone()) {
        return err;
    }

    if (!(err = mIAMCertModule.Init("iam",
              {aos::crypto::KeyTypeEnum::eECDSA, 2,
                  aos::Array<aos::iam::certhandler::ExtendedKeyUsage>(keyUsage, aos::ArraySize(keyUsage))},
              mCryptoProvider, mIAMHSMModule, mStorage))
             .IsNone()) {
        return err;
    }

    if (!(err = mCertHandler.RegisterModule(mIAMCertModule)).IsNone()) {
        return err;
    }

    // Register sm cert module

    if (!(err = mSMHSMModule.Init("sm", {cPKCS11ModuleLibrary, {}, {}, cPKCS11ModuleTokenLabel, cPKCS11ModulePinFile},
              mPKCS11Manager, mCryptoProvider))
             .IsNone()) {
        return err;
    }

    if (!(err = mSMCertModule.Init("sm",
              {aos::crypto::KeyTypeEnum::eECDSA, 2,
                  aos::Array<aos::iam::certhandler::ExtendedKeyUsage>(keyUsage, aos::ArraySize(keyUsage))},
              mCryptoProvider, mSMHSMModule, mStorage))
             .IsNone()) {
        return err;
    }

    if (!(err = mCertHandler.RegisterModule(mSMCertModule)).IsNone()) {
        return err;
    }

    return aos::ErrorEnum::eNone;
}

aos::Error App::InitCommunication()
{
    aos::Error err;

    if (!(err = mTransport.Init(communication::XenVChan::cXSReadPath, communication::XenVChan::cXSWritePath))
             .IsNone()) {
        return err;
    }

    if (!(err = mChannelManager.Init(mTransport)).IsNone()) {
        return err;
    }

    return aos::ErrorEnum::eNone;
}

} // namespace aos::zephyr::app
