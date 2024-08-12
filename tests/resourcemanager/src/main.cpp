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
    aos::sm::resourcemanager::NodeConfig nodeConfig;

    aos::String jsonStr(cTestNodeConfigJSON);
    auto        err = provider.ParseNodeConfig(jsonStr, nodeConfig);

    zassert_true(err.IsNone(), "Failed to parse node config: %s", err.Message());
    zassert_equal(1, nodeConfig.mNodeConfig.mPriority, "Priority mismatch");
    zassert_true(strncmp("1.0", nodeConfig.mVendorVersion.CStr(), nodeConfig.mVendorVersion.Size()) == 0,
        "File content mismatch");
    zassert_true(
        strncmp("mainType", nodeConfig.mNodeConfig.mNodeType.CStr(), nodeConfig.mNodeConfig.mNodeType.Size()) == 0,
        "Node type mismatch");
}

ZTEST(resourcemanager, test_DumpNodeConfig)
{
    aos::Log::SetCallback(TestLogCallback);

    ResourceManagerJSONProvider          provider;
    aos::sm::resourcemanager::NodeConfig nodeConfig;
    nodeConfig.mNodeConfig.mPriority = 1;
    nodeConfig.mVendorVersion        = "1.0";
    nodeConfig.mNodeConfig.mNodeType = "mainType";

    aos::StaticString<1024> jsonStr;
    auto                    err = provider.DumpNodeConfig(nodeConfig, jsonStr);

    zassert_true(err.IsNone(), "Failed to dump node config: %s", err.Message());
    zassert_false(jsonStr.IsEmpty(), "Empty json string");

    aos::sm::resourcemanager::NodeConfig parsednodeConfig;
    err = provider.ParseNodeConfig(jsonStr, parsednodeConfig);

    zassert_true(err.IsNone(), "Failed to parse node config: %s", err.Message());
    zassert_true(nodeConfig.mNodeConfig == parsednodeConfig.mNodeConfig, "Parsed node config mismatch");
    zassert_true(
        nodeConfig.mVendorVersion == parsednodeConfig.mVendorVersion, "Parsed unit config vendor version mismatch");
}
