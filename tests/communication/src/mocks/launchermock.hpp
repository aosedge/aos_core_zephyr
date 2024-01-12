/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LAUNCHERMOCK_HPP_
#define LAUNCHERMOCK_HPP_

#include <condition_variable>
#include <mutex>

#include <aos/common/types.hpp>
#include <aos/sm/launcher.hpp>

class LauncherMock : public aos::sm::launcher::LauncherItf {
public:
    aos::Error RunInstances(const aos::Array<aos::ServiceInfo>& services, const aos::Array<aos::LayerInfo>& layers,
        const aos::Array<aos::InstanceInfo>& instances, bool forceRestart) override
    {
        std::lock_guard<std::mutex> lock(mMutex);

        mServices      = services;
        mLayers        = layers;
        mInstances     = instances;
        mForceRestart  = forceRestart;
        mEventReceived = true;

        mCV.notify_one();

        return aos::ErrorEnum::eNone;
    }

    aos::Error WaitEvent(const std::chrono::duration<double> timeout)
    {
        std::unique_lock<std::mutex> lock(mMutex);

        if (!mCV.wait_for(lock, timeout, [&] { return mEventReceived; })) {
            return aos::ErrorEnum::eTimeout;
        }

        mEventReceived = false;

        return aos::ErrorEnum::eNone;
    }

    const aos::Array<aos::ServiceInfo> GetServices() const
    {
        std::lock_guard<std::mutex> lock(mMutex);
        return mServices;
    }

    const aos::Array<aos::LayerInfo> GetLayers() const
    {
        std::lock_guard<std::mutex> lock(mMutex);
        return mLayers;
    }

    const aos::Array<aos::InstanceInfo> GetInstances() const
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

    aos::ServiceInfoStaticArray  mServices {};
    aos::LayerInfoStaticArray    mLayers {};
    aos::InstanceInfoStaticArray mInstances {};
    bool                         mForceRestart = false;

    std::condition_variable mCV;
    mutable std::mutex      mMutex;
    bool                    mEventReceived = false;
};

#endif
