/*
 * Copyright (C) 2024 Renesas Electronics Corporation.
 * Copyright (C) 2024 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef RESOURCEMANAGERSTUB_HPP_
#define RESOURCEMANAGERSTUB_HPP_

#include <aos/sm/resourcemanager.hpp>

namespace aos::zephyr {

class ResourceManagerStub : public aos::sm::resourcemanager::ResourceManagerItf {
public:
    /**
     * Returns current node config version.
     *
     * @return RetWithError<StaticString<cVersionLen>>.
     */
    RetWithError<StaticString<cVersionLen>> GetNodeConfigVersion() const override { return mVersion; }

    /**
     * Returns node config.
     *
     * @param nodeConfig[out] param to store node config.
     * @return Error.
     */
    Error GetNodeConfig(aos::NodeConfig& nodeConfig) const override
    {
        nodeConfig = *mNodeConfig;

        return ErrorEnum::eNone;
    }

    /**
     * Gets device info by name.
     *
     * @param deviceName device name.
     * @param[out] deviceInfo param to store device info.
     * @return Error.
     */
    Error GetDeviceInfo(const String& deviceName, DeviceInfo& deviceInfo) const override
    {
        (void)deviceName;
        (void)deviceInfo;

        return ErrorEnum::eNone;
    }

    /**
     * Gets resource info by name.
     *
     * @param resourceName resource name.
     * @param[out] resourceInfo param to store resource info.
     * @return Error.
     */
    Error GetResourceInfo(const String& resourceName, ResourceInfo& resourceInfo) const override
    {
        (void)resourceName;
        (void)resourceInfo;

        return ErrorEnum::eNone;
    }

    /**
     * Allocates device by name.
     *
     * @param deviceName device name.
     * @param instanceID instance ID.
     * @return Error.
     */
    Error AllocateDevice(const String& deviceName, const String& instanceID) override
    {
        (void)deviceName;
        (void)instanceID;

        return ErrorEnum::eNone;
    }

    /**
     * Releases device for instance.
     *
     * @param deviceName device name.
     * @param instanceID instance ID.
     * @return Error.
     */
    Error ReleaseDevice(const String& deviceName, const String& instanceID) override
    {
        (void)deviceName;
        (void)instanceID;

        return ErrorEnum::eNone;
    }

    /**
     * Releases all previously allocated devices for instance.
     *
     * @param instanceID instance ID.
     * @return Error.
     */
    Error ReleaseDevices(const String& instanceID) override
    {
        (void)instanceID;

        return ErrorEnum::eNone;
    }

    /**
     * Resets allocated devices.
     *
     * @return Error.
     */
    Error ResetAllocatedDevices() override { return ErrorEnum::eNone; }

    /**
     * Returns ID list of instances that allocate specific device.
     *
     * @param deviceName device name.
     * @param instances[out] param to store instance ID(s).
     * @return Error.
     */
    Error GetDeviceInstances(const String& deviceName, Array<StaticString<cInstanceIDLen>>& instanceIDs) const override
    {
        (void)deviceName;
        (void)instanceIDs;

        return ErrorEnum::eNone;
    }

    /**
     * Checks configuration.
     *
     * @param version config version
     * @param config string with configuration.
     * @return Error.
     */
    Error CheckNodeConfig(const String& version, const String& config) const override
    {
        (void)version;
        (void)config;

        return ErrorEnum::eNone;
    }

    /**
     * Updates configuration.
     *
     * @param version config version.
     * @param config string with configuration.
     * @return Error.
     */
    Error UpdateNodeConfig(const String& version, const String& config) override
    {
        mVersion = version;
        mConfig  = config;

        return aos::ErrorEnum::eNone;
    }

    /**
     * Subscribes to current node config change.
     *
     * @param receiver node config receiver.
     * @return Error.
     */
    Error SubscribeCurrentNodeConfigChange(aos::sm::resourcemanager::NodeConfigReceiverItf& receiver) override
    {
        (void)receiver;

        return ErrorEnum::eNone;
    }

    /**
     * Unsubscribes to current node config change.
     *
     * @param receiver node config receiver.
     * @return Error.
     */
    Error UnsubscribeCurrentNodeConfigChange(aos::sm::resourcemanager::NodeConfigReceiverItf& receiver) override
    {
        (void)receiver;

        return ErrorEnum::eNone;
    }

    /**
     * Sets node config.
     *
     * @param nodeConfig node config.
     */
    void SetNodeConfig(const aos::NodeConfig& nodeConfig) { *mNodeConfig = nodeConfig; }

    /**
     * Returns node config string.
     *
     * @return String
     */
    String GetNodeConfigString() const { return mConfig; }

private:
    StaticString<cVersionLen>                                  mVersion;
    StaticString<aos::sm::resourcemanager::cNodeConfigJSONLen> mConfig;
    std::unique_ptr<aos::NodeConfig>                           mNodeConfig = std::make_unique<aos::NodeConfig>();
};

} // namespace aos::zephyr

#endif
