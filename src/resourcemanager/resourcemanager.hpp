/*
 * Copyright (C) 2024 Renesas Electronics Corporation.
 * Copyright (C) 2024 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef RESOURCE_MANAGER_HPP_
#define RESOURCE_MANAGER_HPP_

#include <zephyr/data/json.h>

#include <aos/common/tools/allocator.hpp>
#include <aos/sm/resourcemanager.hpp>

/**
 * Node config.
 */
struct NodeConfig {
    const char* vendorVersion = "";
    const char* nodeType      = "";
    uint32_t    priority      = 0;
};

class ResourceManagerJSONProvider : public aos::sm::resourcemanager::JSONProviderItf {
public:
    /**
     * Dumps config object into string.
     *
     * @param config config object.
     * @param[out] json json representation of config.
     * @return Error.
     */
    aos::Error DumpNodeConfig(const aos::sm::resourcemanager::NodeConfig& config, aos::String& json) const override;

    /**
     * Parses config object from string.
     *
     * @param json json representation of config.
     * @param[out] config config.
     * @return Error.
     */
    aos::Error ParseNodeConfig(const aos::String& json, aos::sm::resourcemanager::NodeConfig& config) const override;

private:
    static constexpr size_t cJsonMaxContentLen = 1024;
    static constexpr size_t cAllocationSize    = 2048;
    static constexpr size_t cMaxNumAllocations = 32;

    mutable aos::Mutex                                                mMutex;
    mutable aos::StaticString<cJsonMaxContentLen>                     mJSONBuffer;
    mutable aos::StaticAllocator<cAllocationSize, cMaxNumAllocations> mAllocator;
};

/**
 * Host device manager.
 */
class HostDeviceManager : public aos::sm::resourcemanager::HostDeviceManagerItf {
public:
    /**
     * Allocates device for instance.
     *
     * @param deviceInfo device info.
     * @param instanceID instance ID.
     * @return Error.
     */
    aos::Error AllocateDevice(const aos::DeviceInfo& deviceInfo, const aos::String& instanceID) override;

    /**
     * Removes instance from device.
     *
     * @param deviceName device name.
     * @param instanceID instance ID.
     * @return Error.
     */
    aos::Error RemoveInstanceFromDevice(const aos::String& deviceName, const aos::String& instanceID) override;

    /**
     * Removes instance from all devices.
     *
     * @param instanceID instance ID.
     * @return Error.
     */
    aos::Error RemoveInstanceFromAllDevices(const aos::String& instanceID) override;

    /**
     * Returns ID list of instances that allocate specific device.
     *
     * @param deviceName device name.
     * @param instances[out] param to store instance ID(s).
     * @return Error.
     */
    aos::Error GetDeviceInstances(
        const aos::String& deviceName, aos::Array<aos::StaticString<aos::cInstanceIDLen>>& instanceIDs) const override;

    /**
     * Checks if device exists.
     *
     * @param device device name.
     * @return true if device exists, false otherwise.
     */
    bool DeviceExists(const aos::String& device) const override;
};

/**
 * Host group manager.
 */
class HostGroupManager : public aos::sm::resourcemanager::HostGroupManagerItf {
public:
    /**
     * Checks if group exists.
     *
     * @param group group name.
     * @return true if group exists, false otherwise.
     */
    bool GroupExists(const aos::String& group) const override;
};

#endif
