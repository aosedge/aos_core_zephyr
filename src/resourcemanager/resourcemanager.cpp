/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <aos/common/tools/memory.hpp>

#include "log.hpp"
#include "resourcemanager.hpp"

namespace aos::zephyr::resourcemanager {

/***********************************************************************************************************************
 * Static
 **********************************************************************************************************************/

/**
 * Node config.
 */
struct NodeConfig {
    const char* version  = "";
    const char* nodeType = "";
    uint32_t    priority = 0;
};

static const struct json_obj_descr cNodeConfigDescr[] = {
    JSON_OBJ_DESCR_PRIM(NodeConfig, version, JSON_TOK_STRING),
    JSON_OBJ_DESCR_PRIM(NodeConfig, nodeType, JSON_TOK_STRING),
    JSON_OBJ_DESCR_PRIM(NodeConfig, priority, JSON_TOK_NUMBER),
};

/***********************************************************************************************************************
 * JSONProvider
 **********************************************************************************************************************/
/***********************************************************************************************************************
 * Public
 **********************************************************************************************************************/

Error JSONProvider::DumpNodeConfig(const sm::resourcemanager::NodeConfig& config, String& json) const
{
    LockGuard lock(mMutex);

    auto jsonNodeConfig = MakeUnique<NodeConfig>(&mAllocator);

    jsonNodeConfig->version  = config.mVersion.CStr();
    jsonNodeConfig->nodeType = config.mNodeConfig.mNodeType.CStr();
    jsonNodeConfig->priority = config.mNodeConfig.mPriority;

    auto ret = json_obj_encode_buf(
        cNodeConfigDescr, ARRAY_SIZE(cNodeConfigDescr), jsonNodeConfig.Get(), json.Get(), json.MaxSize());
    if (ret < 0) {
        return AOS_ERROR_WRAP(ret);
    }

    auto err = json.Resize(strlen(json.CStr()));
    if (!err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    return ErrorEnum::eNone;
}

Error JSONProvider::ParseNodeConfig(const String& json, sm::resourcemanager::NodeConfig& config) const
{
    LockGuard lock(mMutex);

    // json_object_parse mutates the input string, so we need to copy it
    mJSONBuffer = json;

    auto parsedNodeConfig = MakeUnique<NodeConfig>(&mAllocator);

    auto ret = json_obj_parse(
        mJSONBuffer.Get(), mJSONBuffer.Size(), cNodeConfigDescr, ARRAY_SIZE(cNodeConfigDescr), parsedNodeConfig.Get());
    if (ret < 0) {
        return AOS_ERROR_WRAP(ret);
    }

    config.mVersion              = parsedNodeConfig->version;
    config.mNodeConfig.mNodeType = parsedNodeConfig->nodeType;
    config.mNodeConfig.mPriority = parsedNodeConfig->priority;

    return ErrorEnum::eNone;
}

/***********************************************************************************************************************
 * HostDeviceManager
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * Public
 **********************************************************************************************************************/

Error HostDeviceManager::AllocateDevice(const DeviceInfo& deviceInfo, const String& instanceID)
{
    (void)deviceInfo;
    (void)instanceID;

    return ErrorEnum::eNone;
}

Error HostDeviceManager::RemoveInstanceFromDevice(const String& deviceName, const String& instanceID)
{
    (void)deviceName;
    (void)instanceID;

    return ErrorEnum::eNone;
}

Error HostDeviceManager::RemoveInstanceFromAllDevices(const String& instanceID)
{
    (void)instanceID;

    return ErrorEnum::eNone;
}

Error HostDeviceManager::GetDeviceInstances(
    const String& deviceName, Array<StaticString<cInstanceIDLen>>& instanceIDs) const
{
    (void)deviceName;
    (void)instanceIDs;

    return ErrorEnum::eNone;
}

bool HostDeviceManager::DeviceExists(const String& device) const
{
    (void)device;

    return false;
}

/***********************************************************************************************************************
 * HostGroupManager
 **********************************************************************************************************************/
/***********************************************************************************************************************
 * Public
 **********************************************************************************************************************/

bool HostGroupManager::GroupExists(const String& group) const
{
    (void)group;

    return false;
}

} // namespace aos::zephyr::resourcemanager
