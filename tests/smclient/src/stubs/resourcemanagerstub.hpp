/*
 * Copyright (C) 2024 Renesas Electronics Corporation.
 * Copyright (C) 2024 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef RESOURCEMANAGERSTUB_HPP_
#define RESOURCEMANAGERSTUB_HPP_

#include <aos/sm/resourcemanager.hpp>

class ResourceManagerStub : public aos::sm::resourcemanager::ResourceManagerItf {
public:
    aos::RetWithError<aos::StaticString<aos::cVersionLen>> GetNodeConfigVersion() const override { return mVersion; }

    aos::Error GetDeviceInfo(const aos::String& deviceName, aos::DeviceInfo& deviceInfo) const override
    {
        return aos::ErrorEnum::eNone;
    }

    aos::Error GetResourceInfo(const aos::String& resourceName, aos::ResourceInfo& resourceInfo) const override
    {
        return aos::ErrorEnum::eNone;
    }

    aos::Error AllocateDevice(const aos::String& deviceName, const aos::String& instanceID) override
    {
        return aos::ErrorEnum::eNone;
    }

    aos::Error ReleaseDevice(const aos::String& deviceName, const aos::String& instanceID) override
    {
        return aos::ErrorEnum::eNone;
    }

    aos::Error ReleaseDevices(const aos::String& instanceID) override { return aos::ErrorEnum::eNone; }

    aos::Error GetDeviceInstances(
        const aos::String& deviceName, aos::Array<aos::StaticString<aos::cInstanceIDLen>>& instanceIDs) const override
    {
        return aos::ErrorEnum::eNone;
    }

    aos::Error CheckNodeConfig(const aos::String& version, const aos::String& config) const override
    {
        return aos::ErrorEnum::eNone;
    }

    aos::Error UpdateNodeConfig(const aos::String& version, const aos::String& config) override
    {
        mVersion = version;
        mConfig  = config;

        return aos::ErrorEnum::eNone;
    }

    const aos::String& GetNodeConfig() const { return mConfig; }

private:
    aos::StaticString<aos::cVersionLen>                             mVersion;
    aos::StaticString<aos::sm::resourcemanager::cNodeConfigJSONLen> mConfig;
};

#endif
