
/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/tc_util.h>
#include <zephyr/ztest.h>

#include <aos/common/tools/thread.hpp>

#include "mocklauncher.hpp"

Error MockLauncher::RunInstances(const Array<ServiceInfo>& services, const Array<LayerInfo>& layers,
    const Array<InstanceInfo>& instances, bool forceRestart)
{

    zassert_equal(mExpectedForceRestart, forceRestart, "Incorrect force restart flag");

    zassert_equal(mExpectedServices.Size(), services.Size(), "Incorrect count of services");
    zassert_equal(
        mExpectedServices[0].mVersionInfo.mAosVersion, services[0].mVersionInfo.mAosVersion, "Incorrect Aos version");
    zassert_equal(mExpectedServices[0].mServiceID, services[0].mServiceID, "Incorrect ServiceID");
    zassert_equal(mExpectedServices[0].mURL, services[0].mURL, "Incorrect URL");

    zassert_equal(mExpectedInstances.Size(), instances.Size(), "Incorrect count of instances");
    zassert_equal(mExpectedInstances[0].mInstanceIdent.mServiceID, instances[0].mInstanceIdent.mServiceID,
        "Incorrect service id for instance");
    zassert_equal(mExpectedInstances[0].mInstanceIdent.mSubjectID, instances[0].mInstanceIdent.mSubjectID,
        "Incorrect subject id for instance");
    zassert_equal(
        mExpectedInstances[0].mInstanceIdent.mInstance, instances[0].mInstanceIdent.mInstance, "Incorrect instance");
    zassert_equal(
        mExpectedInstances[0].mPriority, instances[0].mPriority, "Incorrect priority %d", instances[0].mPriority);

    zassert_equal(mExpectedLayers.Size(), layers.Size(), "Incorrect count of layers");

    UniqueLock lock(mWaitMessageMutex);
    mReadyToProcess = true;
    mWaitMessageCondVar.NotifyOne();

    return ErrorEnum::eNone;
}

Error MockLauncher::RunLastInstances()
{
    return ErrorEnum::eNone;
}
