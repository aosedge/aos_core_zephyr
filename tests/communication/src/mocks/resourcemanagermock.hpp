/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef RESOURCEMANAGERMOCK_HPP_
#define RESOURCEMANAGERMOCK_HPP_

#include "resourcemanager/resourcemanager.hpp"

class ResourceManagerMock : public ResourceManagerItf {
public:
    aos::Error GetUnitConfigInfo(aos::String& version) const override
    {
        version = mVersion.c_str();

        return mError;
    }

    aos::Error CheckUnitConfig(const aos::String& version, const aos::String& unitConfig) const override
    {
        return mError;
    };

    aos::Error UpdateUnitConfig(const aos::String& version, const aos::String& unitConfig) override
    {
        mVersion    = version.CStr();
        mUnitConfig = unitConfig.CStr();

        return mError;
    };

    void SetError(const aos::Error& err) { mError = err; }

    const std::string& GetVersion() const { return mVersion; }

    const std::string& GetUnitConfig() const { return mUnitConfig; }

    void Clear()
    {
        mVersion.clear();
        mUnitConfig.clear();
        mError = aos::ErrorEnum::eNone;
    }

private:
    std::string mVersion;
    std::string mUnitConfig;
    aos::Error  mError;
};

#endif
