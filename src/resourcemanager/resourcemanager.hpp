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
     * @param nodeConfig node config object.
     * @param[out] json node config JSON string.
     * @return Error.
     */
    Error NodeConfigToJSON(const sm::resourcemanager::NodeConfig& nodeConfig, String& json) const override;

    /**
     * Creates node config object from a JSON string.
     *
     * @param json node config JSON string.
     * @param[out] nodeConfig node config object.
     * @return Error.
     */
    Error NodeConfigFromJSON(const String& json, sm::resourcemanager::NodeConfig& nodeConfig) const override;

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
     * Checks if device exists.
     *
     * @param device device name.
     * @return true if device exists, false otherwise.
     */
    Error CheckDevice(const String& device) const override;

    /**
     * Checks if group exists.
     *
     * @param group group name.
     * @return true if group exists, false otherwise.
     */
    Error CheckGroup(const String& group) const override;
};

} // namespace aos::zephyr::resourcemanager

#endif
