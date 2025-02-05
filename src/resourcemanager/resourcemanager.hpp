/*
 * Copyright (C) 2024 Renesas Electronics Corporation.
 * Copyright (C) 2024 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef RESOURCEMANAGER_HPP_
#define RESOURCEMANAGER_HPP_

#include <zephyr/data/json.h>

#include <aos/common/tools/allocator.hpp>
#include <aos/sm/resourcemanager.hpp>

namespace aos::zephyr::resourcemanager {

/**
 * JSON provider.
 */
class JSONProvider : public sm::resourcemanager::JSONProviderItf {
public:
    /**
     * Dumps config object into string.
     *
     * @param config config object.
     * @param[out] json json representation of config.
     * @return Error.
     */
    Error DumpNodeConfig(const sm::resourcemanager::NodeConfig& config, String& json) const override;

    /**
     * Parses config object from string.
     *
     * @param json json representation of config.
     * @param[out] config config.
     * @return Error.
     */
    Error ParseNodeConfig(const String& json, sm::resourcemanager::NodeConfig& config) const override;

private:
    mutable Mutex                                                      mMutex;
    mutable StaticString<aos::sm::resourcemanager::cNodeConfigJSONLen> mJSONBuffer;
    mutable StaticAllocator<sizeof(NodeConfig)>                        mAllocator;
};

/**
 * Host device manager.
 */
class HostDeviceManager : public sm::resourcemanager::HostDeviceManagerItf {
public:
    /**
     * Allocates device for instance.
     *
     * @param deviceInfo device info.
     * @param instanceID instance ID.
     * @return Error.
     */
    Error AllocateDevice(const DeviceInfo& deviceInfo, const String& instanceID) override;

    /**
     * Removes instance from device.
     *
     * @param deviceName device name.
     * @param instanceID instance ID.
     * @return Error.
     */
    Error RemoveInstanceFromDevice(const String& deviceName, const String& instanceID) override;

    /**
     * Removes instance from all devices.
     *
     * @param instanceID instance ID.
     * @return Error.
     */
    Error RemoveInstanceFromAllDevices(const String& instanceID) override;

    /**
     * Returns ID list of instances that allocate specific device.
     *
     * @param deviceName device name.
     * @param instances[out] param to store instance ID(s).
     * @return Error.
     */
    Error GetDeviceInstances(const String& deviceName, Array<StaticString<cInstanceIDLen>>& instanceIDs) const override;

    /**
     * Checks if device exists.
     *
     * @param device device name.
     * @return true if device exists, false otherwise.
     */
    bool DeviceExists(const String& device) const override;
};

/**
 * Host group manager.
 */
class HostGroupManager : public sm::resourcemanager::HostGroupManagerItf {
public:
    /**
     * Checks if group exists.
     *
     * @param group group name.
     * @return true if group exists, false otherwise.
     */
    bool GroupExists(const String& group) const override;
};

} // namespace aos::zephyr::resourcemanager

#endif
