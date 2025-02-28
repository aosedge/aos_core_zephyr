/*
 * Copyright (C) 2024 Renesas Electronics Corporation.
 * Copyright (C) 2024 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LAUNCHERSTUB_HPP_
#define LAUNCHERSTUB_HPP_

#include <condition_variable>
#include <mutex>

#include <aos/common/types.hpp>
#include <aos/sm/launcher.hpp>

namespace aos::zephyr {

class LauncherStub : public aos::sm::launcher::LauncherItf {
public:
    /**
     * Runs specified instances.
     *
     * @param instances instance info array.
     * @param forceRestart forces restart already started instance.
     * @return Error.
     */
    Error RunInstances(const Array<ServiceInfo>& services, const Array<LayerInfo>& layers,
        const Array<InstanceInfo>& instances, bool forceRestart) override
    {
        std::lock_guard<std::mutex> lock(mMutex);

        mServices      = services;
        mLayers        = layers;
        mInstances     = instances;
        mForceRestart  = forceRestart;
        mEventReceived = true;

        mCV.notify_one();

        return ErrorEnum::eNone;
    }

    /**
     * Returns current run status.
     *
     * @param instances instances status.
     * @return Error.
     */
    Error GetCurrentRunStatus(Array<InstanceStatus>& instances) const override
    {
        (void)instances;

        return ErrorEnum::eNone;
    }

    /**
     * Overrides environment variables for specified instances.
     *
     * @param envVarsInfo environment variables info.
     * @param statuses[out] environment variables statuses.
     * @return Error
     */
    Error OverrideEnvVars(const Array<cloudprotocol::EnvVarsInstanceInfo>& envVarsInfo,
        Array<cloudprotocol::EnvVarsInstanceStatus>&                       statuses) override
    {
        (void)envVarsInfo;
        (void)statuses;

        return ErrorEnum::eNone;
    }

    Error WaitEvent(const std::chrono::duration<double> timeout)
    {
        std::unique_lock<std::mutex> lock(mMutex);

        if (!mCV.wait_for(lock, timeout, [&] { return mEventReceived; })) {
            return ErrorEnum::eTimeout;
        }

        mEventReceived = false;

        return ErrorEnum::eNone;
    }

    const Array<ServiceInfo> GetServices() const
    {
        std::lock_guard<std::mutex> lock(mMutex);
        return mServices;
    }

    const Array<LayerInfo> GetLayers() const
    {
        std::lock_guard<std::mutex> lock(mMutex);
        return mLayers;
    }

    const Array<InstanceInfo> GetInstances() const
    {
        std::lock_guard<std::mutex> lock(mMutex);
        return mInstances;
    }

    bool IsForceRestart() const
    {
        std::lock_guard<std::mutex> lock(mMutex);
        return mForceRestart;
    }

    void Clear()
    {
        std::lock_guard<std::mutex> lock(mMutex);

        mEventReceived = false;
        mServices.Clear();
        mLayers.Clear();
        mInstances.Clear();
        mForceRestart = false;
    }

private:
    static constexpr auto cWaitTimeout = std::chrono::seconds {1};

    ServiceInfoStaticArray  mServices {};
    LayerInfoStaticArray    mLayers {};
    InstanceInfoStaticArray mInstances {};
    bool                    mForceRestart = false;

    std::condition_variable mCV;
    mutable std::mutex      mMutex;
    bool                    mEventReceived = false;
};

} // namespace aos::zephyr

#endif
