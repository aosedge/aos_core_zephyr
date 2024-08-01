/*
 * Copyright (C) 2024 Renesas Electronics Corporation.
 * Copyright (C) 2024 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>

#include <aos/common/tools/log.hpp>
#include <aos/common/tools/string.hpp>
#include <aos/common/types.hpp>

#include "resourcemanager/resourcemanager.hpp"

static constexpr auto cTestNodeConfigJSON = R"({"nodeType":"mainType","priority":1,"vendorVersion":"1.0"})";

static void TestLogCallback(const char* module, aos::LogLevel level, const aos::String& message)
{
    printk("[resourcemanager] %s \n", message.CStr());
}

ZTEST_SUITE(resourcemanager, NULL, NULL, NULL, NULL, NULL);

ZTEST(resourcemanager, test_JSONProviderParse)
{
    aos::Log::SetCallback(TestLogCallback);

    ResourceManagerJSONProvider          provider;
    aos::sm::resourcemanager::UnitConfig unitConfig;

    aos::String jsonStr(cTestNodeConfigJSON);
    auto        err = provider.ParseNodeUnitConfig(jsonStr, unitConfig);

    zassert_true(err.IsNone(), "Failed to parse node unit config: %s", err.Message());
    zassert_equal(1, unitConfig.mNodeUnitConfig.mPriority, "Priority mismatch");
    zassert_true(strncmp("1.0", unitConfig.mVendorVersion.CStr(), unitConfig.mVendorVersion.Size()) == 0,
        "File content mismatch");
    zassert_true(
        strncmp("mainType", unitConfig.mNodeUnitConfig.mNodeType.CStr(), unitConfig.mNodeUnitConfig.mNodeType.Size())
            == 0,
        "Node type mismatch");
}

ZTEST(resourcemanager, test_DumpUnitConfig)
{
    aos::Log::SetCallback(TestLogCallback);

    ResourceManagerJSONProvider          provider;
    aos::sm::resourcemanager::UnitConfig unitConfig;
    unitConfig.mNodeUnitConfig.mPriority = 1;
    unitConfig.mVendorVersion            = "1.0";
    unitConfig.mNodeUnitConfig.mNodeType = "mainType";

    aos::StaticString<1024> jsonStr;
    auto                    err = provider.DumpUnitConfig(unitConfig, jsonStr);

    zassert_true(err.IsNone(), "Failed to dump node unit config: %s", err.Message());
    zassert_false(jsonStr.IsEmpty(), "Empty json string");

    aos::sm::resourcemanager::UnitConfig parsedUnitConfig;
    err = provider.ParseNodeUnitConfig(jsonStr, parsedUnitConfig);

    zassert_true(err.IsNone(), "Failed to parse node unit config: %s", err.Message());
    zassert_true(unitConfig.mNodeUnitConfig == parsedUnitConfig.mNodeUnitConfig, "Parsed node unit config mismatch");
    zassert_true(
        unitConfig.mVendorVersion == parsedUnitConfig.mVendorVersion, "Parsed unit config vendor version mismatch");
}
