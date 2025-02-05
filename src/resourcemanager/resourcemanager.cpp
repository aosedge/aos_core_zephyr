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

namespace {

/**
 * Device.
 */
struct Device {
    const char* name;
    const char* hostDevices[cMaxNumHostDevices];
    size_t      hostDevicesLen;
    int         sharedCount;
    const char* groups[cMaxNumGroups];
    size_t      groupsLen;
};

/**
 * FS mount.
 */
struct FSMount {
    const char* destination;
    const char* source;
    const char* type;
    const char* options[cFSMountMaxNumOptions];
    size_t      optionsLen;
};

/**
 * Host.
 */
struct Host {
    const char* ip;
    const char* hostName;
};

/**
 * Resource.
 */
struct Resource {
    const char* name;
    const char* groups[cMaxNumGroups];
    size_t      groupsLen;
    FSMount     mounts[cMaxNumFSMounts];
    size_t      mountsLen;
    const char* env[cMaxNumEnvVariables];
    size_t      envLen;
    Host        hosts[cMaxNumHosts];
    size_t      hostsLen;
};

/**
 * Node config.
 */
struct NodeConfig {
    // cppcheck-suppress unusedStructMember
    const char* version;
    // cppcheck-suppress unusedStructMember
    const char* nodeType;
    Device      devices[cMaxNumDevices];
    size_t      devicesLen;
    Resource    resources[cMaxNumNodeResources];
    size_t      resourcesLen;
    // cppcheck-suppress unusedStructMember
    const char* labels[cMaxNumNodeLabels];
    // cppcheck-suppress unusedStructMember
    size_t labelsLen;
    // cppcheck-suppress unusedStructMember
    uint32_t priority;
};

} // namespace

/**
 * Device json object descriptor.
 */
static const struct json_obj_descr cDeviceDescr[] = {
    JSON_OBJ_DESCR_PRIM(Device, name, JSON_TOK_STRING),
    JSON_OBJ_DESCR_ARRAY(Device, hostDevices, cMaxNumHostDevices, hostDevicesLen, JSON_TOK_STRING),
    JSON_OBJ_DESCR_PRIM(Device, sharedCount, JSON_TOK_NUMBER),
    JSON_OBJ_DESCR_ARRAY(Device, groups, cMaxNumGroups, groupsLen, JSON_TOK_STRING),
};

/**
 * FS mount json object descriptor.
 */
static const struct json_obj_descr cFSMountDescr[] = {
    JSON_OBJ_DESCR_PRIM(FSMount, destination, JSON_TOK_STRING),
    JSON_OBJ_DESCR_PRIM(FSMount, source, JSON_TOK_STRING),
    JSON_OBJ_DESCR_PRIM(FSMount, type, JSON_TOK_STRING),
    JSON_OBJ_DESCR_ARRAY(FSMount, options, cFSMountMaxNumOptions, optionsLen, JSON_TOK_STRING),
};

/**
 * Host json object descriptor.
 */
static const struct json_obj_descr cHostDescr[] = {
    JSON_OBJ_DESCR_PRIM(Host, ip, JSON_TOK_STRING),
    JSON_OBJ_DESCR_PRIM(Host, hostName, JSON_TOK_STRING),
};

/**
 * Resource json object descriptor.
 */
static const struct json_obj_descr cResourceDescr[] = {
    JSON_OBJ_DESCR_PRIM(Resource, name, JSON_TOK_STRING),
    JSON_OBJ_DESCR_ARRAY(Resource, groups, cMaxNumGroups, groupsLen, JSON_TOK_STRING),
    JSON_OBJ_DESCR_OBJ_ARRAY(Resource, mounts, cMaxNumFSMounts, mountsLen, cFSMountDescr, ARRAY_SIZE(cFSMountDescr)),
    JSON_OBJ_DESCR_ARRAY(Resource, env, cMaxNumEnvVariables, envLen, JSON_TOK_STRING),
    JSON_OBJ_DESCR_OBJ_ARRAY(Resource, hosts, cMaxNumHosts, hostsLen, cHostDescr, ARRAY_SIZE(cHostDescr)),
};

/**
 * Node config json object descriptor.
 */
static const struct json_obj_descr cNodeConfigDescr[] = {
    JSON_OBJ_DESCR_PRIM(NodeConfig, version, JSON_TOK_STRING),
    JSON_OBJ_DESCR_PRIM(NodeConfig, nodeType, JSON_TOK_STRING),
    JSON_OBJ_DESCR_OBJ_ARRAY(NodeConfig, devices, cMaxNumDevices, devicesLen, cDeviceDescr, ARRAY_SIZE(cDeviceDescr)),
    JSON_OBJ_DESCR_OBJ_ARRAY(
        NodeConfig, resources, cMaxNumNodeResources, resourcesLen, cResourceDescr, ARRAY_SIZE(cResourceDescr)),
    JSON_OBJ_DESCR_ARRAY(NodeConfig, labels, cMaxNumNodeLabels, labelsLen, JSON_TOK_STRING),
    JSON_OBJ_DESCR_PRIM(NodeConfig, priority, JSON_TOK_NUMBER),
};

/***********************************************************************************************************************
 * Private
 **********************************************************************************************************************/

static void FillJSONStruct(const Array<DeviceInfo>& inDevices, NodeConfig& jsonNodeConfig)
{
    jsonNodeConfig.devicesLen = inDevices.Size();

    for (size_t i = 0; i < inDevices.Size(); ++i) {
        const auto& inDevice   = inDevices[i];
        auto&       jsonDevice = jsonNodeConfig.devices[i];

        jsonDevice.name           = inDevice.mName.CStr();
        jsonDevice.sharedCount    = inDevice.mSharedCount;
        jsonDevice.groupsLen      = inDevice.mGroups.Size();
        jsonDevice.hostDevicesLen = inDevice.mHostDevices.Size();

        for (size_t j = 0; j < inDevice.mGroups.Size(); ++j) {
            jsonDevice.groups[j] = inDevice.mGroups[j].CStr();
        }

        for (size_t j = 0; j < inDevice.mHostDevices.Size(); ++j) {
            jsonDevice.hostDevices[j] = inDevice.mHostDevices[j].CStr();
        }
    }
}

static void FillJSONStruct(const Array<ResourceInfo>& inResources, NodeConfig& jsonNodeConfig)
{
    jsonNodeConfig.resourcesLen = inResources.Size();

    for (size_t i = 0; i < inResources.Size(); ++i) {
        const auto& inResource   = inResources[i];
        auto&       jsonResource = jsonNodeConfig.resources[i];

        jsonResource.name      = inResource.mName.CStr();
        jsonResource.groupsLen = inResource.mGroups.Size();

        for (size_t j = 0; j < inResource.mGroups.Size(); ++j) {
            jsonResource.groups[j] = inResource.mGroups[j].CStr();
        }

        jsonResource.mountsLen = inResource.mMounts.Size();

        for (size_t j = 0; j < inResource.mMounts.Size(); ++j) {
            const auto& inMount  = inResource.mMounts[j];
            auto&       outMount = jsonResource.mounts[j];

            outMount.destination = inMount.mDestination.CStr();
            outMount.source      = inMount.mSource.CStr();
            outMount.type        = inMount.mType.CStr();
            outMount.optionsLen  = inMount.mOptions.Size();

            for (size_t k = 0; k < inMount.mOptions.Size(); ++k) {
                outMount.options[k] = inMount.mOptions[k].CStr();
            }
        }

        jsonResource.envLen = inResource.mEnv.Size();

        for (size_t j = 0; j < inResource.mEnv.Size(); ++j) {
            jsonResource.env[j] = inResource.mEnv[j].CStr();
        }

        jsonResource.hostsLen = inResource.mHosts.Size();

        for (size_t j = 0; j < inResource.mHosts.Size(); ++j) {
            const auto& inHost  = inResource.mHosts[j];
            auto&       outHost = jsonResource.hosts[j];

            outHost.ip       = inHost.mIP.CStr();
            outHost.hostName = inHost.mHostname.CStr();
        }
    }
}

static Error FillAosStruct(const NodeConfig& in, Array<DeviceInfo>& outDevices)
{
    if (auto err = outDevices.Resize(in.devicesLen); !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    for (size_t i = 0; i < in.devicesLen; ++i) {
        const auto& inDeviceInfo  = in.devices[i];
        DeviceInfo& outDeviceInfo = outDevices[i];

        outDeviceInfo.mName        = inDeviceInfo.name;
        outDeviceInfo.mSharedCount = inDeviceInfo.sharedCount;

        for (size_t j = 0; j < inDeviceInfo.groupsLen; ++j) {
            if (auto err = outDeviceInfo.mGroups.PushBack(inDeviceInfo.groups[j]); !err.IsNone()) {
                return AOS_ERROR_WRAP(err);
            }
        }

        for (size_t j = 0; j < inDeviceInfo.hostDevicesLen; ++j) {
            if (auto err = outDeviceInfo.mHostDevices.PushBack(inDeviceInfo.hostDevices[j]); !err.IsNone()) {
                return AOS_ERROR_WRAP(err);
            }
        }
    }

    return ErrorEnum::eNone;
}

static Error FillAosStruct(const NodeConfig& in, Array<ResourceInfo>& outResources)
{
    if (auto err = outResources.Resize(in.resourcesLen); !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    for (size_t i = 0; i < in.resourcesLen; ++i) {
        const auto&   inResourceInfo  = in.resources[i];
        ResourceInfo& outResourceInfo = outResources[i];

        outResourceInfo.mName = inResourceInfo.name;

        for (size_t j = 0; j < inResourceInfo.groupsLen; ++j) {
            if (auto err = outResourceInfo.mGroups.PushBack(inResourceInfo.groups[j]); !err.IsNone()) {
                return AOS_ERROR_WRAP(err);
            }
        }

        if (auto err = outResourceInfo.mMounts.Resize(inResourceInfo.mountsLen); !err.IsNone()) {
            return AOS_ERROR_WRAP(err);
        }

        for (size_t j = 0; j < inResourceInfo.mountsLen; ++j) {
            const auto& parsedMount = inResourceInfo.mounts[j];

            FileSystemMount& mount = outResourceInfo.mMounts[j];

            mount.mDestination = parsedMount.destination;
            mount.mType        = parsedMount.type;
            mount.mSource      = parsedMount.source;

            for (size_t k = 0; k < parsedMount.optionsLen; ++k) {
                if (auto err = mount.mOptions.PushBack(parsedMount.options[k]); !err.IsNone()) {
                    return AOS_ERROR_WRAP(err);
                }
            }
        }

        for (size_t j = 0; j < inResourceInfo.envLen; ++j) {
            if (auto err = outResourceInfo.mEnv.PushBack(inResourceInfo.env[j]); !err.IsNone()) {
                return AOS_ERROR_WRAP(err);
            }
        }

        if (auto err = outResourceInfo.mHosts.Resize(inResourceInfo.hostsLen); !err.IsNone()) {
            return AOS_ERROR_WRAP(err);
        }

        for (size_t j = 0; j < inResourceInfo.hostsLen; ++j) {
            outResourceInfo.mHosts[j].mIP       = inResourceInfo.hosts[j].ip;
            outResourceInfo.mHosts[j].mHostname = inResourceInfo.hosts[j].hostName;
        }
    }

    return ErrorEnum::eNone;
}

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

    FillJSONStruct(config.mNodeConfig.mDevices, *jsonNodeConfig);
    FillJSONStruct(config.mNodeConfig.mResources, *jsonNodeConfig);

    jsonNodeConfig->labelsLen = config.mNodeConfig.mLabels.Size();
    for (size_t i = 0; i < config.mNodeConfig.mLabels.Size(); ++i) {
        jsonNodeConfig->labels[i] = config.mNodeConfig.mLabels[i].CStr();
    }

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
    *parsedNodeConfig     = {};

    auto ret = json_obj_parse(
        mJSONBuffer.Get(), mJSONBuffer.Size(), cNodeConfigDescr, ARRAY_SIZE(cNodeConfigDescr), parsedNodeConfig.Get());
    if (ret < 0) {
        return AOS_ERROR_WRAP(ret);
    }

    config.mVersion              = parsedNodeConfig->version;
    config.mNodeConfig.mNodeType = parsedNodeConfig->nodeType;
    config.mNodeConfig.mPriority = parsedNodeConfig->priority;

    if (auto err = FillAosStruct(*parsedNodeConfig, config.mNodeConfig.mDevices); !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    if (auto err = FillAosStruct(*parsedNodeConfig, config.mNodeConfig.mResources); !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    for (size_t i = 0; i < parsedNodeConfig->labelsLen; ++i) {
        if (auto err = config.mNodeConfig.mLabels.PushBack(parsedNodeConfig->labels[i]); !err.IsNone()) {
            return AOS_ERROR_WRAP(err);
        }
    }

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
