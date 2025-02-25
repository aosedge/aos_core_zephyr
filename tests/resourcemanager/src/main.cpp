/*
 * Copyright (C) 2024 Renesas Electronics Corporation.
 * Copyright (C) 2024 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>

#include <aos/common/tools/string.hpp>
#include <aos/common/types.hpp>

#include "resourcemanager/resourcemanager.hpp"

#include "utils/log.hpp"
#include "utils/utils.hpp"

using namespace aos::zephyr;

/***********************************************************************************************************************
 * Consts
 **********************************************************************************************************************/

static constexpr auto cTestNodeConfigJSON = R"({
    "devices": [
        {
            "groups": [
                "group1",
                "group2"
            ],
            "hostDevices": [
                "hostDevice1",
                "hostDevice2"
            ],
            "name": "device1",
            "sharedCount": 1
        },
        {
            "groups": [
                "group3",
                "group4"
            ],
            "hostDevices": [
                "hostDevice3",
                "hostDevice4"
            ],
            "name": "device2",
            "sharedCount": 2
        }
    ],
    "resources": [
        {
            "name": "resource1",
            "groups": ["g1", "g2"],
            "mounts": [
                {
                    "destination": "d1",
                    "type": "type1",
                    "source": "source1",
                    "options": ["option1", "option2"]
                },
                {
                    "destination": "d2",
                    "type": "type2",
                    "source": "source2",
                    "options": ["option3", "option4"]
                }
            ],
            "env": ["env1", "env2"],
            "hosts": [
                {
                    "ip": "10.0.0.100",
                    "hostName": "host1"
                },
                {
                    "ip": "10.0.0.101",
                    "hostName": "host2"
                }
            ]
        },
        {
            "name": "resource2",
            "groups": ["g3", "g4"],
            "mounts": [
                {
                    "destination": "d3",
                    "type": "type3",
                    "source": "source3",
                    "options": ["option5", "option6"]
                },
                {
                    "destination": "d4",
                    "type": "type4",
                    "source": "source4",
                    "options": ["option7", "option8"]
                }
            ],
            "env": ["env3", "env4"],
            "hosts": [
                {
                    "ip": "10.0.0.102",
                    "hostName": "host3"
                },
                {
                    "ip": "10.0.0.103",
                    "hostName": "host4"
                }
            ]
        }
    ],
    "labels": [
        "mainNode"
    ],
    "nodeType": "mainType",
    "priority": 1,
    "version": "1.0.0"
}

)";

/***********************************************************************************************************************
 * Static
 **********************************************************************************************************************/

static aos::sm::resourcemanager::NodeConfig CreateNodeConfig()
{
    aos::sm::resourcemanager::NodeConfig nodeConfig;

    nodeConfig.mVersion              = "1.0.0";
    nodeConfig.mNodeConfig.mPriority = 1;
    nodeConfig.mNodeConfig.mNodeType = "mainType";

    nodeConfig.mNodeConfig.mDevices.Resize(2);

    nodeConfig.mNodeConfig.mDevices[0].mName        = "device1";
    nodeConfig.mNodeConfig.mDevices[0].mSharedCount = 1;
    nodeConfig.mNodeConfig.mDevices[0].mGroups.PushBack("group1");
    nodeConfig.mNodeConfig.mDevices[0].mGroups.PushBack("group2");
    nodeConfig.mNodeConfig.mDevices[0].mHostDevices.PushBack("hostDevice1");
    nodeConfig.mNodeConfig.mDevices[0].mHostDevices.PushBack("hostDevice2");

    nodeConfig.mNodeConfig.mDevices[1].mName        = "device2";
    nodeConfig.mNodeConfig.mDevices[1].mSharedCount = 2;
    nodeConfig.mNodeConfig.mDevices[1].mGroups.PushBack("group3");
    nodeConfig.mNodeConfig.mDevices[1].mGroups.PushBack("group4");
    nodeConfig.mNodeConfig.mDevices[1].mHostDevices.PushBack("hostDevice3");
    nodeConfig.mNodeConfig.mDevices[1].mHostDevices.PushBack("hostDevice4");

    nodeConfig.mNodeConfig.mResources.Resize(2);

    nodeConfig.mNodeConfig.mResources[0].mName = "resource1";
    nodeConfig.mNodeConfig.mResources[0].mGroups.PushBack("g1");
    nodeConfig.mNodeConfig.mResources[0].mGroups.PushBack("g2");

    nodeConfig.mNodeConfig.mResources[0].mMounts.Resize(2);
    nodeConfig.mNodeConfig.mResources[0].mMounts[0].mDestination = "d1";
    nodeConfig.mNodeConfig.mResources[0].mMounts[0].mType        = "type1";
    nodeConfig.mNodeConfig.mResources[0].mMounts[0].mSource      = "source1";
    nodeConfig.mNodeConfig.mResources[0].mMounts[0].mOptions.PushBack("option1");
    nodeConfig.mNodeConfig.mResources[0].mMounts[0].mOptions.PushBack("option2");

    nodeConfig.mNodeConfig.mResources[0].mMounts[1].mDestination = "d2";
    nodeConfig.mNodeConfig.mResources[0].mMounts[1].mType        = "type2";
    nodeConfig.mNodeConfig.mResources[0].mMounts[1].mSource      = "source2";
    nodeConfig.mNodeConfig.mResources[0].mMounts[1].mOptions.PushBack("option3");
    nodeConfig.mNodeConfig.mResources[0].mMounts[1].mOptions.PushBack("option4");

    nodeConfig.mNodeConfig.mResources[0].mEnv.PushBack("env1");
    nodeConfig.mNodeConfig.mResources[0].mEnv.PushBack("env2");

    nodeConfig.mNodeConfig.mResources[0].mHosts.Resize(2);
    nodeConfig.mNodeConfig.mResources[0].mHosts[0].mIP       = "10.0.0.100";
    nodeConfig.mNodeConfig.mResources[0].mHosts[0].mHostname = "host1";

    nodeConfig.mNodeConfig.mResources[0].mHosts[1].mIP       = "10.0.0.101";
    nodeConfig.mNodeConfig.mResources[0].mHosts[1].mHostname = "host2";

    nodeConfig.mNodeConfig.mResources[1].mName = "resource2";
    nodeConfig.mNodeConfig.mResources[1].mGroups.PushBack("g3");
    nodeConfig.mNodeConfig.mResources[1].mGroups.PushBack("g4");

    nodeConfig.mNodeConfig.mResources[1].mMounts.Resize(2);
    nodeConfig.mNodeConfig.mResources[1].mMounts[0].mDestination = "d3";
    nodeConfig.mNodeConfig.mResources[1].mMounts[0].mType        = "type3";
    nodeConfig.mNodeConfig.mResources[1].mMounts[0].mSource      = "source3";
    nodeConfig.mNodeConfig.mResources[1].mMounts[0].mOptions.PushBack("option5");
    nodeConfig.mNodeConfig.mResources[1].mMounts[0].mOptions.PushBack("option6");

    nodeConfig.mNodeConfig.mResources[1].mMounts[1].mDestination = "d4";
    nodeConfig.mNodeConfig.mResources[1].mMounts[1].mType        = "type4";
    nodeConfig.mNodeConfig.mResources[1].mMounts[1].mSource      = "source4";
    nodeConfig.mNodeConfig.mResources[1].mMounts[1].mOptions.PushBack("option7");
    nodeConfig.mNodeConfig.mResources[1].mMounts[1].mOptions.PushBack("option8");

    nodeConfig.mNodeConfig.mResources[1].mEnv.PushBack("env3");
    nodeConfig.mNodeConfig.mResources[1].mEnv.PushBack("env4");

    nodeConfig.mNodeConfig.mResources[1].mHosts.Resize(2);
    nodeConfig.mNodeConfig.mResources[1].mHosts[0].mIP       = "10.0.0.102";
    nodeConfig.mNodeConfig.mResources[1].mHosts[0].mHostname = "host3";
    nodeConfig.mNodeConfig.mResources[1].mHosts[1].mIP       = "10.0.0.103";
    nodeConfig.mNodeConfig.mResources[1].mHosts[1].mHostname = "host4";

    nodeConfig.mNodeConfig.mLabels.PushBack("mainNode");

    return nodeConfig;
}

static void CompareNodeConfig(const aos::sm::resourcemanager::NodeConfig& nodeConfig,
    const aos::sm::resourcemanager::NodeConfig&                           expectedNodeConfig)
{
    zassert_equal(nodeConfig.mVersion, expectedNodeConfig.mVersion, "Version mismatch");
    zassert_equal(nodeConfig.mNodeConfig.mNodeType, expectedNodeConfig.mNodeConfig.mNodeType, "Node type mismatch");
    zassert_equal(nodeConfig.mNodeConfig.mPriority, expectedNodeConfig.mNodeConfig.mPriority, "Priority mismatch");

    zassert_equal(nodeConfig.mNodeConfig.mDevices, expectedNodeConfig.mNodeConfig.mDevices, "Device info mismatch");
    zassert_equal(nodeConfig.mNodeConfig.mLabels, expectedNodeConfig.mNodeConfig.mLabels, "Node labels mismatch");

    // Compare resources

    zassert_equal(nodeConfig.mNodeConfig.mResources.Size(), expectedNodeConfig.mNodeConfig.mResources.Size(),
        "Resources size mismatch");

    for (size_t i = 0; i < nodeConfig.mNodeConfig.mResources.Size(); ++i) {
        const auto& resource         = nodeConfig.mNodeConfig.mResources[i];
        const auto& expectedResource = expectedNodeConfig.mNodeConfig.mResources[i];

        zassert_equal(resource.mName, expectedResource.mName, "Resource name mismatch");
        zassert_equal(resource.mGroups, expectedResource.mGroups, "Resource groups mismatch");
        zassert_equal(resource.mMounts, expectedResource.mMounts, "Resource mounts mismatch");
        zassert_equal(resource.mEnv, expectedResource.mEnv, "Resource envs mismatch");
        zassert_equal(resource.mHosts, expectedResource.mHosts, "Resource hosts mismatch");
    }
}

/***********************************************************************************************************************
 * Tests
 **********************************************************************************************************************/

ZTEST_SUITE(
    resourcemanager, NULL, NULL, [](void*) { aos::Log::SetCallback(TestLogCallback); }, NULL, NULL);

ZTEST(resourcemanager, test_JSONProviderParse)
{
    aos::zephyr::resourcemanager::JSONProvider provider;
    aos::sm::resourcemanager::NodeConfig       parsedNodeConfig;

    aos::String jsonStr(cTestNodeConfigJSON);
    auto        err = provider.NodeConfigFromJSON(jsonStr, parsedNodeConfig);

    zassert_true(err.IsNone(), "Failed to parse node config: %s", utils::ErrorToCStr(err));

    CompareNodeConfig(parsedNodeConfig, CreateNodeConfig());
}

ZTEST(resourcemanager, test_DumpNodeConfig)
{
    aos::zephyr::resourcemanager::JSONProvider provider;
    aos::sm::resourcemanager::NodeConfig       nodeConfig = CreateNodeConfig();

    aos::StaticString<aos::sm::resourcemanager::cNodeConfigJSONLen> jsonStr;
    auto err = provider.NodeConfigToJSON(nodeConfig, jsonStr);

    zassert_true(err.IsNone(), "Failed to dump node config: %s", utils::ErrorToCStr(err));
    zassert_false(jsonStr.IsEmpty(), "Empty json string");

    aos::sm::resourcemanager::NodeConfig parsedNodeConfig;
    err = provider.NodeConfigFromJSON(jsonStr, parsedNodeConfig);

    zassert_true(err.IsNone(), "Failed to parse node config: %s", utils::ErrorToCStr(err));

    CompareNodeConfig(parsedNodeConfig, nodeConfig);
}
