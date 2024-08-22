/*
 * Copyright (C) 2024 Renesas Electronics Corporation.
 * Copyright (C) 2024 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <memory>

#include <zephyr/tc_util.h>
#include <zephyr/ztest.h>

#include "smclient/smclient.hpp"

#include "stubs/channelmanagerstub.hpp"
#include "stubs/clocksyncstub.hpp"
#include "stubs/nodeinfoproviderstub.hpp"
#include "stubs/resourcemanagerstub.hpp"
#include "utils/log.hpp"
#include "utils/pbmessages.hpp"
#include "utils/utils.hpp"

using namespace aos::zephyr;

/***********************************************************************************************************************
 * Consts
 **********************************************************************************************************************/

static constexpr auto cWaitTimeout = std::chrono::seconds {5};

/***********************************************************************************************************************
 * Types
 **********************************************************************************************************************/

struct smclient_fixture {
    NodeInfoProviderStub                mNodeInfoProvider;
    ResourceManagerStub                 mResourceManager;
    ClockSyncStub                       mClockSync;
    ChannelManagerStub                  mChannelManager;
    std::unique_ptr<smclient::SMClient> mSMClient;
};

/***********************************************************************************************************************
 * Static
 **********************************************************************************************************************/

static aos::Error ReceiveSMOutgoingMessage(ChannelStub* channel, servicemanager_v4_SMOutgoingMessages& message)
{
    return ReceivePBMessage(channel, cWaitTimeout, &message, servicemanager_v4_SMOutgoingMessages_size,
        &servicemanager_v4_SMOutgoingMessages_msg);
}

static aos::Error SendSMIncomingMessage(ChannelStub* channel, const servicemanager_v4_SMIncomingMessages& message)
{
    return SendPBMessage(
        channel, &message, servicemanager_v4_SMIncomingMessages_size, &servicemanager_v4_SMIncomingMessages_msg);
}

static void ReceiveNodeConfigStatus(
    ChannelStub* channel, const aos::NodeInfo& nodeInfo, const aos::String& nodeConfigVersion)
{
    servicemanager_v4_SMOutgoingMessages outgoingMessage;
    auto&                                pbNodeConfigStatus = outgoingMessage.SMOutgoingMessage.node_config_status;

    pbNodeConfigStatus = servicemanager_v4_NodeConfigStatus servicemanager_v4_NodeConfigStatus_init_default;

    auto err = ReceiveSMOutgoingMessage(channel, outgoingMessage);
    zassert_true(err.IsNone(), "Error receiving message: %s", utils::ErrorToCStr(err));

    zassert_equal(outgoingMessage.which_SMOutgoingMessage, servicemanager_v4_SMOutgoingMessages_node_config_status_tag,
        "Unexpected message type");
    zassert_false(pbNodeConfigStatus.has_error, "Unexpected error received");
    zassert_equal(pbNodeConfigStatus.node_id, nodeInfo.mNodeID, "Wrong node ID");
    zassert_equal(pbNodeConfigStatus.node_type, nodeInfo.mNodeType, "Wrong node type");
    zassert_equal(pbNodeConfigStatus.version, nodeConfigVersion, "Wrong node config version");
}

static aos::RetWithError<ChannelStub*> InitSMClient(smclient_fixture* fixture, uint32_t port,
    const aos::NodeInfo& nodeInfo = {}, const aos::String& nodeConfigVersion = "1.0.0")
{
    fixture->mNodeInfoProvider.SetNodeInfo(nodeInfo);
    fixture->mResourceManager.UpdateNodeConfig(nodeConfigVersion, "");

    auto err = fixture->mSMClient->Init(
        fixture->mNodeInfoProvider, fixture->mResourceManager, fixture->mClockSync, fixture->mChannelManager);
    zassert_true(err.IsNone(), "Can't initialize SM client: %s", utils::ErrorToCStr(err));

    fixture->mSMClient->OnClockSynced();

    auto channel = fixture->mChannelManager.GetChannel(port);
    if (!channel.mError.IsNone()) {
        return {nullptr, channel.mError};
    }

    if (port == CONFIG_AOS_SM_SECURE_PORT) {
        ReceiveNodeConfigStatus(channel.mValue, nodeInfo, nodeConfigVersion);
    }

    return channel;
}

/***********************************************************************************************************************
 * Setup
 **********************************************************************************************************************/

ZTEST_SUITE(
    smclient, nullptr,
    []() -> void* {
        aos::Log::SetCallback(TestLogCallback);

        auto fixture = new smclient_fixture;

        return fixture;
    },
    [](void* fixture) {
        auto smclientFixture = static_cast<smclient_fixture*>(fixture);

        smclientFixture->mClockSync.Clear();
        smclientFixture->mSMClient.reset(new smclient::SMClient);
    },
    [](void* fixture) { static_cast<smclient_fixture*>(fixture)->mSMClient.reset(); },
    [](void* fixture) { delete static_cast<smclient_fixture*>(fixture); });

/***********************************************************************************************************************
 * Tests
 **********************************************************************************************************************/

ZTEST_F(smclient, test_ClockSync)
{
    auto [channel, err] = InitSMClient(fixture, CONFIG_AOS_SM_OPEN_PORT);
    zassert_true(err.IsNone(), "Getting channel error: %s", utils::ErrorToCStr(err));

    // Wait clock sync start

    err = fixture->mClockSync.WaitEvent(cWaitTimeout);
    zassert_true(err.IsNone(), "Error waiting clock sync start: %s", utils::ErrorToCStr(err));

    zassert_true(fixture->mClockSync.GetStarted());

    // Wait clock sync request

    err = fixture->mSMClient->SendClockSyncRequest();

    zassert_true(err.IsNone(), "Error sending clock sync request: %s", utils::ErrorToCStr(err));

    servicemanager_v4_SMOutgoingMessages outgoingMessage;
    auto&                                pbClockSyncRequest = outgoingMessage.SMOutgoingMessage.clock_sync_request;

    pbClockSyncRequest = servicemanager_v4_ClockSyncRequest servicemanager_v4_ClockSyncRequest_init_default;

    err = ReceiveSMOutgoingMessage(channel, outgoingMessage);
    zassert_true(err.IsNone(), "Error receiving message: %s", utils::ErrorToCStr(err));

    zassert_equal(outgoingMessage.which_SMOutgoingMessage, servicemanager_v4_SMOutgoingMessages_clock_sync_request_tag);

    // Send clock sync

    auto timestamp = aos::Time::Now();

    servicemanager_v4_SMIncomingMessages incomingMessage;
    auto&                                pbClockSync = incomingMessage.SMIncomingMessage.clock_sync;

    auto unixTime = timestamp.UnixTime();

    incomingMessage.which_SMIncomingMessage = servicemanager_v4_SMIncomingMessages_clock_sync_tag;
    pbClockSync                             = servicemanager_v4_ClockSync servicemanager_v4_ClockSync_init_default;

    incomingMessage.SMIncomingMessage.clock_sync.has_current_time     = true;
    incomingMessage.SMIncomingMessage.clock_sync.current_time.seconds = unixTime.tv_sec;
    incomingMessage.SMIncomingMessage.clock_sync.current_time.nanos   = unixTime.tv_nsec;

    err = SendSMIncomingMessage(channel, incomingMessage);
    zassert_true(err.IsNone(), "Error sending message: %s", utils::ErrorToCStr(err));

    err = fixture->mClockSync.WaitEvent(cWaitTimeout);
    zassert_true(err.IsNone(), "Error waiting clock sync start: %s", utils::ErrorToCStr(err));

    zassert_equal(fixture->mClockSync.GetSyncTime(), timestamp, "Wrong timestamp");
}

ZTEST_F(smclient, test_GetNodeConfigStatus)
{
    const auto    nodeConfigVersion = "1.0.0";
    aos::NodeInfo nodeInfo {.mNodeID = "nodeID", .mNodeType = "nodeType", .mStatus = aos::NodeStatusEnum::eProvisioned};

    auto [channel, err] = InitSMClient(fixture, CONFIG_AOS_SM_SECURE_PORT, nodeInfo, nodeConfigVersion);
    zassert_true(err.IsNone(), "Getting channel error: %s", utils::ErrorToCStr(err));

    // Send get node config status

    servicemanager_v4_SMIncomingMessages incomingMessage;
    auto& pbGetNodeConfigStatus = incomingMessage.SMIncomingMessage.get_node_config_status;

    incomingMessage.which_SMIncomingMessage = servicemanager_v4_SMIncomingMessages_get_node_config_status_tag;
    pbGetNodeConfigStatus = servicemanager_v4_GetNodeConfigStatus servicemanager_v4_GetNodeConfigStatus_init_default;

    err = SendSMIncomingMessage(channel, incomingMessage);
    zassert_true(err.IsNone(), "Error sending message: %s", utils::ErrorToCStr(err));

    // Receive node config status

    ReceiveNodeConfigStatus(channel, nodeInfo, nodeConfigVersion);
}

ZTEST_F(smclient, test_CheckNodeConfig)
{
    aos::NodeInfo nodeInfo {.mNodeID = "nodeID", .mNodeType = "nodeType", .mStatus = aos::NodeStatusEnum::eProvisioned};

    auto [channel, err] = InitSMClient(fixture, CONFIG_AOS_SM_SECURE_PORT, nodeInfo);
    zassert_true(err.IsNone(), "Getting channel error: %s", utils::ErrorToCStr(err));

    // Send get node config status

    servicemanager_v4_SMIncomingMessages incomingMessage;
    auto&                                pbCheckNodeConfig = incomingMessage.SMIncomingMessage.check_node_config;

    incomingMessage.which_SMIncomingMessage = servicemanager_v4_SMIncomingMessages_check_node_config_tag;
    pbCheckNodeConfig = servicemanager_v4_CheckNodeConfig servicemanager_v4_CheckNodeConfig_init_default;

    utils::StringFromCStr(pbCheckNodeConfig.node_config) = "check node config";
    utils::StringFromCStr(pbCheckNodeConfig.version)     = "2.0.0";

    err = SendSMIncomingMessage(channel, incomingMessage);
    zassert_true(err.IsNone(), "Error sending message: %s", utils::ErrorToCStr(err));

    // Receive node config status

    ReceiveNodeConfigStatus(channel, nodeInfo, pbCheckNodeConfig.version);
}

ZTEST_F(smclient, test_SetNodeConfig)
{
    aos::NodeInfo nodeInfo {.mNodeID = "nodeID", .mNodeType = "nodeType", .mStatus = aos::NodeStatusEnum::eProvisioned};

    auto [channel, err] = InitSMClient(fixture, CONFIG_AOS_SM_SECURE_PORT, nodeInfo);
    zassert_true(err.IsNone(), "Getting channel error: %s", utils::ErrorToCStr(err));

    // Send get node config status

    servicemanager_v4_SMIncomingMessages incomingMessage;
    auto&                                pbSetNodeConfig = incomingMessage.SMIncomingMessage.set_node_config;

    incomingMessage.which_SMIncomingMessage = servicemanager_v4_SMIncomingMessages_set_node_config_tag;
    pbSetNodeConfig = servicemanager_v4_SetNodeConfig servicemanager_v4_SetNodeConfig_init_default;

    utils::StringFromCStr(pbSetNodeConfig.node_config) = "set node config";
    utils::StringFromCStr(pbSetNodeConfig.version)     = "2.0.0";

    err = SendSMIncomingMessage(channel, incomingMessage);
    zassert_true(err.IsNone(), "Error sending message: %s", utils::ErrorToCStr(err));

    // Receive node config status

    ReceiveNodeConfigStatus(channel, nodeInfo, pbSetNodeConfig.version);
    zassert_equal(fixture->mResourceManager.GetNodeConfig(), pbSetNodeConfig.node_config, "Wrong node config");
}
