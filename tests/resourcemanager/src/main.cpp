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

#include "utils/log.hpp"
#include "utils/utils.hpp"

using namespace aos::zephyr;

/***********************************************************************************************************************
 * Consts
 **********************************************************************************************************************/

static constexpr auto cTestNodeConfigJSON = R"({"nodeType":"mainType","priority":1,"version":"1.0.0"})";

/***********************************************************************************************************************
 * Tests
 **********************************************************************************************************************/

ZTEST_SUITE(resourcemanager, NULL, NULL, NULL, NULL, NULL);

ZTEST(resourcemanager, test_JSONProviderParse)
{
    aos::Log::SetCallback(TestLogCallback);

    aos::zephyr::resourcemanager::JSONProvider provider;
    aos::sm::resourcemanager::NodeConfig       nodeConfig;

    aos::String jsonStr(cTestNodeConfigJSON);
    auto        err = provider.ParseNodeConfig(jsonStr, nodeConfig);

    zassert_true(err.IsNone(), "Failed to parse node config: %s", err.Message());
    zassert_equal(1, nodeConfig.mNodeConfig.mPriority, "Priority mismatch");
    zassert_equal(nodeConfig.mVersion, "1.0.0", "File content mismatch");
    zassert_equal(nodeConfig.mNodeConfig.mNodeType, "mainType", "Node type mismatch");
}

ZTEST(resourcemanager, test_DumpNodeConfig)
{
    aos::Log::SetCallback(TestLogCallback);

    aos::zephyr::resourcemanager::JSONProvider provider;
    aos::sm::resourcemanager::NodeConfig       nodeConfig;
    nodeConfig.mNodeConfig.mPriority = 1;
    nodeConfig.mVersion              = "1.0.0";
    nodeConfig.mNodeConfig.mNodeType = "mainType";

    aos::StaticString<1024> jsonStr;
    auto                    err = provider.DumpNodeConfig(nodeConfig, jsonStr);

    zassert_true(err.IsNone(), "Failed to dump node config: %s", utils::ErrorToCStr(err));
    zassert_false(jsonStr.IsEmpty(), "Empty json string");

    aos::sm::resourcemanager::NodeConfig parsedNodeConfig;
    err = provider.ParseNodeConfig(jsonStr, parsedNodeConfig);

    zassert_true(err.IsNone(), "Failed to parse node config: %s", utils::ErrorToCStr(err));
    zassert_equal(nodeConfig.mNodeConfig, parsedNodeConfig.mNodeConfig, "Parsed node config mismatch");
    zassert_equal(nodeConfig.mVersion, parsedNodeConfig.mVersion, "Parsed unit config vendor version mismatch");
}
