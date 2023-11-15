/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef APP_HPP_
#define APP_HPP_

#include <aos/common/resourcemonitor.hpp>
#include <aos/common/tools/error.hpp>
#include <aos/common/tools/noncopyable.hpp>
#include <aos/sm/launcher.hpp>
#include <aos/sm/servicemanager.hpp>

#include "clocksync/clocksync.hpp"
#include "cmclient/cmclient.hpp"
#include "ocispec/ocispec.hpp"
#include "resourceusageprovider/resourceusageprovider.hpp"
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
    static App                              sApp;
    CMClient                                mCMClient;
    Storage                                 mStorage;
    Runner                                  mRunner;
    aos::sm::launcher::Launcher             mLauncher;
    ResourceManager                         mResourceManager;
    Downloader                              mDownloader;
    OCISpec                                 mJsonOciSpec;
    aos::sm::servicemanager::ServiceManager mServiceManager;
    aos::monitoring::ResourceMonitor        mResourceMonitor;
    ResourceUsageProvider                   mResourceUsageProvider;
    ClockSync                               mClockSync;
};

#endif
