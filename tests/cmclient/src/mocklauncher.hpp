/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <aos/sm/launcher.hpp>

using namespace aos;

static constexpr size_t TestArraysSize = 10;

class MockLauncher : public sm::launcher::LauncherItf {
public:
    MockLauncher(aos::ConditionalVariable& waitMessageCondVar, aos::Mutex& waitMessageMutex, bool& readyToProcess)
        : mWaitMessageCondVar(waitMessageCondVar)
        , mWaitMessageMutex(waitMessageMutex)
        , mReadyToProcess(readyToProcess)
    {
    }

    Error RunInstances(const Array<ServiceInfo>& services, const Array<LayerInfo>& layers,
        const Array<InstanceInfo>& instances, bool forceRestart) override;

    Error RunLastInstances() override;

    StaticArray<ServiceInfo, TestArraysSize>  mExpectedServices;
    StaticArray<LayerInfo, TestArraysSize>    mExpectedLayers;
    StaticArray<InstanceInfo, TestArraysSize> mExpectedInstances;
    bool                                      mExpectedForceRestart;
    aos::ConditionalVariable&                 mWaitMessageCondVar;
    aos::Mutex&                               mWaitMessageMutex;
    bool&                                     mReadyToProcess;
};
