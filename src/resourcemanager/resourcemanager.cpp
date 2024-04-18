/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <aos/common/tools/memory.hpp>

#include "log.hpp"
#include "resourcemanager.hpp"

/***********************************************************************************************************************
 * Static
 **********************************************************************************************************************/

static const struct json_obj_descr cUnitConfigDescr[] = {
    JSON_OBJ_DESCR_PRIM(UnitConfig, vendorVersion, JSON_TOK_STRING),
    JSON_OBJ_DESCR_PRIM(UnitConfig, nodeType, JSON_TOK_STRING),
    JSON_OBJ_DESCR_PRIM(UnitConfig, priority, JSON_TOK_NUMBER),
};

/***********************************************************************************************************************
 * ResourceManagerJSONProvider
 **********************************************************************************************************************/
/***********************************************************************************************************************
 * Public
 **********************************************************************************************************************/

// cppcheck-suppress unusedFunction
aos::Error ResourceManagerJSONProvider::DumpUnitConfig(
    const aos::sm::resourcemanager::UnitConfig& nodeUnitConfig, aos::String& json) const
{
    aos::LockGuard lock(mMutex);

    mAllocator.Clear();

    auto jsonUnitConfig = aos::MakeUnique<UnitConfig>(&mAllocator);

    jsonUnitConfig->vendorVersion = nodeUnitConfig.mVendorVersion.CStr();
    jsonUnitConfig->nodeType      = nodeUnitConfig.mNodeUnitConfig.mNodeType.CStr();
    jsonUnitConfig->priority      = nodeUnitConfig.mNodeUnitConfig.mPriority;

    auto ret = json_obj_encode_buf(
        cUnitConfigDescr, ARRAY_SIZE(cUnitConfigDescr), jsonUnitConfig.Get(), json.Get(), json.MaxSize());
    if (ret < 0) {
        return AOS_ERROR_WRAP(ret);
    }

    auto err = json.Resize(strlen(json.CStr()));
    if (!err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    return aos::ErrorEnum::eNone;
}

// cppcheck-suppress unusedFunction
aos::Error ResourceManagerJSONProvider::ParseNodeUnitConfig(
    const aos::String& json, aos::sm::resourcemanager::UnitConfig& nodeUnitConfig) const
{
    aos::LockGuard lock(mMutex);

    mAllocator.Clear();

    // json_object_parse mutates the input string, so we need to copy it
    mJSONBuffer = json;

    auto parsedUnitConfig = aos::MakeUnique<UnitConfig>(&mAllocator);

    auto ret = json_obj_parse(
        mJSONBuffer.Get(), mJSONBuffer.Size(), cUnitConfigDescr, ARRAY_SIZE(cUnitConfigDescr), parsedUnitConfig.Get());
    if (ret < 0) {
        return AOS_ERROR_WRAP(ret);
    }

    nodeUnitConfig.mVendorVersion            = parsedUnitConfig->vendorVersion;
    nodeUnitConfig.mNodeUnitConfig.mNodeType = parsedUnitConfig->nodeType;
    nodeUnitConfig.mNodeUnitConfig.mPriority = parsedUnitConfig->priority;

    return aos::ErrorEnum::eNone;
}

/***********************************************************************************************************************
 * HostDeviceManager
 **********************************************************************************************************************/
/***********************************************************************************************************************
 * Public
 **********************************************************************************************************************/

// cppcheck-suppress unusedFunction
aos::Error HostDeviceManager::AllocateDevice(const aos::DeviceInfo& deviceInfo, const aos::String& instanceID)
{
    (void)deviceInfo;
    (void)instanceID;

    return aos::ErrorEnum::eNone;
}

// cppcheck-suppress unusedFunction
aos::Error HostDeviceManager::RemoveInstanceFromDevice(const aos::String& deviceName, const aos::String& instanceID)
{
    (void)deviceName;
    (void)instanceID;

    return aos::ErrorEnum::eNone;
}

// cppcheck-suppress unusedFunction
aos::Error HostDeviceManager::RemoveInstanceFromAllDevices(const aos::String& instanceID)
{
    (void)instanceID;

    return aos::ErrorEnum::eNone;
}

// cppcheck-suppress unusedFunction
aos::Error HostDeviceManager::GetDeviceInstances(
    const aos::String& deviceName, aos::Array<aos::StaticString<aos::cInstanceIDLen>>& instanceIDs) const
{
    (void)deviceName;
    (void)instanceIDs;

    return aos::ErrorEnum::eNone;
}

// cppcheck-suppress unusedFunction
bool HostDeviceManager::DeviceExists(const aos::String& device) const
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

// cppcheck-suppress unusedFunction
bool HostGroupManager::GroupExists(const aos::String& group) const
{
    (void)group;

    return false;
}
