/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "app.hpp"
#include "log.hpp"

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

    if (!(err = mDownloader.Init(mCommunication)).IsNone()) {
        return err;
    }

    if (!(err = mServiceManager.Init(mJsonOciSpec, mDownloader, mStorage)).IsNone()) {
        return err;
    }

    if (!(err = mResourceUsageProvider.Init()).IsNone()) {
        return err;
    }

    if (!(err = mResourceMonitor.Init(mResourceUsageProvider, mCommunication, mCommunication)).IsNone()) {
        return err;
    }

    if (!(err = mLauncher.Init(
              mServiceManager, mRunner, mJsonOciSpec, mCommunication, mStorage, mResourceMonitor, mCommunication))
             .IsNone()) {
        return err;
    }

    if (!(err = mCommOpenChannel.Init(VChannel::cXSOpenReadPath, VChannel::cXSOpenWritePath)).IsNone()) {
        return err;
    }

    if (!(err = mCommSecureChannel.Init(VChannel::cXSCloseReadPath, VChannel::cXSCloseWritePath)).IsNone()) {
        return err;
    }

    if (!(err = mClockSync.Init(mCommunication)).IsNone()) {
        return err;
    }

    if (!(err = mCommunication.Init(mCommOpenChannel, mCommSecureChannel, mLauncher, mResourceManager, mResourceMonitor,
              mDownloader, mClockSync))
             .IsNone()) {
        return err;
    }

    return aos::ErrorEnum::eNone;
}
