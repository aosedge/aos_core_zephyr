/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef RESOURCEMANAGERSTUB_HPP_
#define RESOURCEMANAGERSTUB_HPP_

#include <aos/sm/resourcemanager.hpp>

class ResourceManagerStub : public aos::sm::resourcemanager::ResourceManagerItf {
public:
    aos::RetWithError<aos::StaticString<aos::cVersionLen>> GetVersion() const override { return {mVersion, mError}; }

    aos::Error CheckUnitConfig(const aos::String& version, const aos::String& unitConfig) const override
    {
        return mError;
    };

    aos::Error UpdateUnitConfig(const aos::String& version, const aos::String& unitConfig) override
    {
        mVersion    = version;
        mUnitConfig = unitConfig.CStr();

        return mError;
    };

    aos::Error GetDeviceInfo(const aos::String& deviceName, aos::DeviceInfo& deviceInfo) const override
    {
        (void)deviceName;
        (void)deviceInfo;

        return mError;
    }

    aos::Error GetResourceInfo(const aos::String& resourceName, aos::ResourceInfo& resourceInfo) const override
    {
        (void)resourceName;
        (void)resourceInfo;

        return mError;
    }

    aos::Error AllocateDevice(const aos::String& deviceName, const aos::String& instanceID) override
    {
        (void)deviceName;
        (void)instanceID;

        return mError;
    }

    aos::Error ReleaseDevice(const aos::String& deviceName, const aos::String& instanceID) override
    {
        (void)deviceName;
        (void)instanceID;

        return mError;
    }

    aos::Error ReleaseDevices(const aos::String& instanceID) override
    {
        (void)instanceID;

        return mError;
    }
    aos::Error GetDeviceInstances(
        const aos::String& deviceName, aos::Array<aos::StaticString<aos::cInstanceIDLen>>& instanceIDs) const override
    {
        (void)deviceName;
        (void)instanceIDs;

        return mError;
    }

    void SetError(const aos::Error& err) { mError = err; }

    const std::string& GetUnitConfig() const { return mUnitConfig; }

    void Clear()
    {
        mVersion.Clear();
        mUnitConfig.clear();
        mError = aos::ErrorEnum::eNone;
    }

private:
    aos::StaticString<aos::cVersionLen> mVersion;
    std::string                         mUnitConfig;
    aos::Error                          mError;
};

#endif
