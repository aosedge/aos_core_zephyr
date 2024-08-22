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
#include "stubs/resourcemonitorstub.hpp"
#include "utils/log.hpp"
#include "utils/pbconvert.hpp"
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
    ResourceMonitorStub                 mResourceMonitor;
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

    auto err = fixture->mSMClient->Init(fixture->mNodeInfoProvider, fixture->mResourceManager,
        fixture->mResourceMonitor, fixture->mClockSync, fixture->mChannelManager);
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

static void PBToMonitoringData(
    const servicemanager_v4_MonitoringData& pbMonitoring, aos::monitoring::MonitoringData& aosMonitoring)
{
    aosMonitoring.mRAM = pbMonitoring.ram;
    aosMonitoring.mCPU = pbMonitoring.cpu;
    aosMonitoring.mDisk.Resize(pbMonitoring.disk_count);

    for (size_t i = 0; i < pbMonitoring.disk_count; i++) {
        aosMonitoring.mDisk[i].mName     = utils::StringFromCStr(pbMonitoring.disk[i].name);
        aosMonitoring.mDisk[i].mUsedSize = pbMonitoring.disk[i].used_size;
    }

    aosMonitoring.mDownload = pbMonitoring.download;
    aosMonitoring.mUpload   = pbMonitoring.upload;
}

template <typename T>
static void PBToNodeMonitoring(const T& pbNodeMonitoring, aos::monitoring::NodeMonitoringData& nodeMonitoring)
{
    if (pbNodeMonitoring.has_node_monitoring) {
        if (pbNodeMonitoring.node_monitoring.has_timestamp) {
            nodeMonitoring.mTimestamp = aos::Time::Unix(
                pbNodeMonitoring.node_monitoring.timestamp.seconds, pbNodeMonitoring.node_monitoring.timestamp.nanos);
        }

        PBToMonitoringData(pbNodeMonitoring.node_monitoring, nodeMonitoring.mMonitoringData);
    }

    nodeMonitoring.mServiceInstances.Resize(pbNodeMonitoring.instances_monitoring_count);

    for (size_t i = 0; i < pbNodeMonitoring.instances_monitoring_count; i++) {
        utils::PBToInstanceIdent(
            pbNodeMonitoring.instances_monitoring[i].instance, nodeMonitoring.mServiceInstances[i].mInstanceIdent);
        PBToMonitoringData(pbNodeMonitoring.instances_monitoring[i].monitoring_data,
            nodeMonitoring.mServiceInstances[i].mMonitoringData);
    }
}

static void GenerateNodeMonitoring(aos::monitoring::NodeMonitoringData& nodeMonitoring)
{
    aos::PartitionInfo nodeDisks[]     = {{.mName = "disk1", .mUsedSize = 512}, {.mName = "disk2", .mUsedSize = 1024}};
    aos::PartitionInfo instanceDisks[] = {{.mName = "disk3", .mUsedSize = 2048}};

    aos::monitoring::InstanceMonitoringData instancesData[] = {
        {{"service1", "subject1", 1},
            {2000, 34423, aos::Array<aos::PartitionInfo>(instanceDisks, aos::ArraySize(instanceDisks)), 342, 432}},
        {{"service2", "subject2", 2},
            {3000, 54344, aos::Array<aos::PartitionInfo>(instanceDisks, aos::ArraySize(instanceDisks)), 432, 321}},
        {{"service3", "subject3", 3},
            {4000, 90995, aos::Array<aos::PartitionInfo>(instanceDisks, aos::ArraySize(instanceDisks)), 594, 312}},
    };

    nodeMonitoring.mTimestamp = aos::Time::Now();
    nodeMonitoring.mMonitoringData
        = {1000, 2048, aos::Array<aos::PartitionInfo>(nodeDisks, aos::ArraySize(nodeDisks)), 4, 5};
    nodeMonitoring.mServiceInstances
        = aos::Array<aos::monitoring::InstanceMonitoringData>(instancesData, aos::ArraySize(instancesData));
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

ZTEST_F(smclient, test_InstantMonitoring)
{
    auto [channel, err] = InitSMClient(fixture, CONFIG_AOS_SM_SECURE_PORT,
        {.mNodeID = "nodeID", .mNodeType = "nodeType", .mStatus = aos::NodeStatusEnum::eProvisioned});
    zassert_true(err.IsNone(), "Getting channel error: %s", utils::ErrorToCStr(err));

    // Send node monitoring

    aos::monitoring::NodeMonitoringData sendMonitoring;

    GenerateNodeMonitoring(sendMonitoring);

    err = fixture->mSMClient->SendMonitoringData(sendMonitoring);
    zassert_true(err.IsNone(), "Getting channel error: %s", utils::ErrorToCStr(err));

    // Receive monitoring data

    servicemanager_v4_SMOutgoingMessages outgoingMessage;
    auto&                                pbInstantMonitoring = outgoingMessage.SMOutgoingMessage.instant_monitoring;

    pbInstantMonitoring = servicemanager_v4_InstantMonitoring servicemanager_v4_InstantMonitoring_init_default;

    err = ReceiveSMOutgoingMessage(channel, outgoingMessage);
    zassert_true(err.IsNone(), "Error receiving message: %s", utils::ErrorToCStr(err));

    zassert_equal(outgoingMessage.which_SMOutgoingMessage, servicemanager_v4_SMOutgoingMessages_instant_monitoring_tag,
        "Unexpected message type");

    aos::monitoring::NodeMonitoringData receiveMonitoring;

    PBToNodeMonitoring(pbInstantMonitoring, receiveMonitoring);
    zassert_equal(receiveMonitoring, sendMonitoring, "Wrong node monitoring");
}

ZTEST_F(smclient, test_GetAverageMonitoring)
{
    auto [channel, err] = InitSMClient(fixture, CONFIG_AOS_SM_SECURE_PORT,
        {.mNodeID = "nodeID", .mNodeType = "nodeType", .mStatus = aos::NodeStatusEnum::eProvisioned});
    zassert_true(err.IsNone(), "Getting channel error: %s", utils::ErrorToCStr(err));

    // Send get average monitoring

    aos::monitoring::NodeMonitoringData sendMonitoring;

    GenerateNodeMonitoring(sendMonitoring);

    fixture->mResourceMonitor.SetMonitoringData(sendMonitoring);

    servicemanager_v4_SMIncomingMessages incomingMessage;
    auto& pbGetAverageMonitoring = incomingMessage.SMIncomingMessage.get_average_monitoring;

    incomingMessage.which_SMIncomingMessage = servicemanager_v4_SMIncomingMessages_get_average_monitoring_tag;
    pbGetAverageMonitoring = servicemanager_v4_GetAverageMonitoring servicemanager_v4_GetAverageMonitoring_init_default;

    err = SendSMIncomingMessage(channel, incomingMessage);
    zassert_true(err.IsNone(), "Error sending message: %s", utils::ErrorToCStr(err));

    // Receive monitoring data

    servicemanager_v4_SMOutgoingMessages outgoingMessage;
    auto&                                pbAverageMonitoring = outgoingMessage.SMOutgoingMessage.average_monitoring;

    pbAverageMonitoring = servicemanager_v4_AverageMonitoring servicemanager_v4_AverageMonitoring_init_default;

    err = ReceiveSMOutgoingMessage(channel, outgoingMessage);
    zassert_true(err.IsNone(), "Error receiving message: %s", utils::ErrorToCStr(err));

    zassert_equal(outgoingMessage.which_SMOutgoingMessage, servicemanager_v4_SMOutgoingMessages_average_monitoring_tag,
        "Unexpected message type");

    aos::monitoring::NodeMonitoringData receiveMonitoring;

    PBToNodeMonitoring(pbAverageMonitoring, receiveMonitoring);
    zassert_equal(receiveMonitoring, sendMonitoring, "Wrong node monitoring");
}
