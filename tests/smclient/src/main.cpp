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
#include "stubs/connectionsubscriberstub.hpp"
#include "stubs/downloaderstub.hpp"
#include "stubs/launcherstub.hpp"
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
    LauncherStub                        mLauncher;
    ResourceManagerStub                 mResourceManager;
    ResourceMonitorStub                 mResourceMonitor;
    DownloaderStub                      mDownloader;
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

    auto err = fixture->mSMClient->Init(fixture->mNodeInfoProvider, fixture->mLauncher, fixture->mResourceManager,
        fixture->mResourceMonitor, fixture->mDownloader, fixture->mClockSync, fixture->mChannelManager);
    zassert_true(err.IsNone(), "Can't initialize SM client: %s", utils::ErrorToCStr(err));

    err = fixture->mSMClient->Start();
    zassert_true(err.IsNone(), "Can't start SM client: %s", utils::ErrorToCStr(err));

    fixture->mSMClient->OnClockSynced();

    auto channel = fixture->mChannelManager.GetChannel(port, cWaitTimeout);
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
    aosMonitoring.mPartitions.Resize(pbMonitoring.partitions_count);

    for (size_t i = 0; i < pbMonitoring.partitions_count; i++) {
        aosMonitoring.mPartitions[i].mName     = utils::StringFromCStr(pbMonitoring.partitions[i].name);
        aosMonitoring.mPartitions[i].mUsedSize = pbMonitoring.partitions[i].used_size;
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

static void PBToInstanceStatus(const servicemanager_v4_InstanceStatus& pbInstance, aos::InstanceStatus& aosInstance)
{
    if (pbInstance.has_instance) {
        utils::PBToInstanceIdent(pbInstance.instance, aosInstance.mInstanceIdent);
    }

    aosInstance.mServiceVersion = utils::StringFromCStr(pbInstance.service_version);
    utils::PBToEnum(pbInstance.run_state, aosInstance.mRunState);

    aosInstance.mError = utils::PBToError(pbInstance.error_info);

    if (pbInstance.has_error_info) {
        if (pbInstance.error_info.exit_code) {
            aosInstance.mError = pbInstance.error_info.exit_code;
        } else {
            aosInstance.mError = static_cast<aos::ErrorEnum>(pbInstance.error_info.aos_code);
        }
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

static void GenerateServicesInfo(aos::Array<aos::ServiceInfo>& services)
{
    uint8_t sha256[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

    services.EmplaceBack(aos::ServiceInfo {
        "service1", "provider1", "1.0.0", 1, "service1 URL", aos::Array<uint8_t>(sha256, sizeof(sha256)), 312});
    services.EmplaceBack(aos::ServiceInfo {
        "service2", "provider2", "2.0.0", 2, "service2 URL", aos::Array<uint8_t>(sha256, sizeof(sha256)), 512});
}

static void GenerateLayersInfo(aos::Array<aos::LayerInfo>& layers)
{
    uint8_t sha256[] = {9, 8, 7, 6, 5, 4, 3, 2, 1, 0};

    layers.EmplaceBack(
        aos::LayerInfo {"layer1", "digest1", "1.0.0", "layer1 URL", aos::Array<uint8_t>(sha256, sizeof(sha256)), 1024});
    layers.EmplaceBack(
        aos::LayerInfo {"layer2", "digest2", "2.0.0", "layer2 URL", aos::Array<uint8_t>(sha256, sizeof(sha256)), 2048});
    layers.EmplaceBack(
        aos::LayerInfo {"layer3", "digest3", "3.0.0", "layer3 URL", aos::Array<uint8_t>(sha256, sizeof(sha256)), 4096});
}

static void GenerateInstancesInfo(aos::Array<aos::InstanceInfo>& instances)
{
    instances.EmplaceBack(aos::InstanceInfo {{"service1", "subject1", 1}, 1, 1, "storage1", "state1"});
    instances.EmplaceBack(aos::InstanceInfo {{"service2", "subject2", 2}, 2, 2, "storage2", "state2"});
    instances.EmplaceBack(aos::InstanceInfo {{"service3", "subject3", 3}, 3, 3, "storage3", "state3"});
    instances.EmplaceBack(aos::InstanceInfo {{"service4", "subject4", 4}, 4, 4, "storage4", "state4"});
}

static void GenerateInstancesRunStatus(aos::Array<aos::InstanceStatus>& status)
{
    status.EmplaceBack(aos::InstanceStatus {
        {"service1", "subject1", 0}, "1.0.0", aos::InstanceRunStateEnum::eActive, aos::ErrorEnum::eNone});
    status.EmplaceBack(
        aos::InstanceStatus {{"service1", "subject1", 1}, "3.0.0", aos::InstanceRunStateEnum::eFailed, 42});
    status.EmplaceBack(aos::InstanceStatus {
        {"service1", "subject1", 2}, "2.0.0", aos::InstanceRunStateEnum::eFailed, aos::ErrorEnum::eRuntime});
}

static void GenerateImageContentInfo(downloader::ImageContentInfo& contentInfo)

{
    uint8_t sha256[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

    contentInfo.mRequestID = 42;
    contentInfo.mFiles.EmplaceBack(
        downloader::FileInfo {"path/to/file1", aos::Array<uint8_t>(sha256, sizeof(sha256)), 1024});
    contentInfo.mFiles.EmplaceBack(
        downloader::FileInfo {"path/to/file2", aos::Array<uint8_t>(sha256, sizeof(sha256)), 2048});
    contentInfo.mFiles.EmplaceBack(
        downloader::FileInfo {"path/to/file3", aos::Array<uint8_t>(sha256, sizeof(sha256)), 4096});
    contentInfo.mError = aos::Error(aos::ErrorEnum::eRuntime, "some error");
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

        smclientFixture->mLauncher.Clear();
        smclientFixture->mDownloader.Clear();
        smclientFixture->mClockSync.Clear();
        smclientFixture->mNodeInfoProvider.Clear();
        smclientFixture->mSMClient.reset(new smclient::SMClient);
    },
    [](void* fixture) {
        auto smclientFixture = static_cast<smclient_fixture*>(fixture);

        auto err = smclientFixture->mSMClient->Stop();
        zassert_true(err.IsNone(), "Can't stop SM client: %s", utils::ErrorToCStr(err));

        smclientFixture->mSMClient.reset();
    },
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

ZTEST_F(smclient, test_RunInstance)
{
    auto [channel, err] = InitSMClient(fixture, CONFIG_AOS_SM_SECURE_PORT,
        {.mNodeID = "nodeID", .mNodeType = "nodeType", .mStatus = aos::NodeStatusEnum::eProvisioned});
    zassert_true(err.IsNone(), "Getting channel error: %s", utils::ErrorToCStr(err));

    // Send run instances

    aos::ServiceInfoStaticArray  services;
    aos::LayerInfoStaticArray    layers;
    aos::InstanceInfoStaticArray instances;

    GenerateServicesInfo(services);
    GenerateLayersInfo(layers);
    GenerateInstancesInfo(instances);

    servicemanager_v4_SMIncomingMessages incomingMessage;
    auto&                                pbRunInstances = incomingMessage.SMIncomingMessage.run_instances;

    incomingMessage.which_SMIncomingMessage = servicemanager_v4_SMIncomingMessages_run_instances_tag;
    pbRunInstances = servicemanager_v4_RunInstances servicemanager_v4_RunInstances_init_default;

    pbRunInstances.services_count = services.Size();

    for (size_t i = 0; i < services.Size(); i++) {
        const auto& aosService = services[i];
        auto&       pbService  = pbRunInstances.services[i];

        utils::StringFromCStr(pbService.service_id)  = aosService.mServiceID;
        utils::StringFromCStr(pbService.provider_id) = aosService.mProviderID;
        utils::StringFromCStr(pbService.version)     = aosService.mVersion;
        pbService.gid                                = aosService.mGID;
        utils::StringFromCStr(pbService.url)         = aosService.mURL;
        utils::ByteArrayToPB(aosService.mSHA256, pbService.sha256);
        pbService.size = aosService.mSize;
    }

    pbRunInstances.layers_count = layers.Size();

    for (size_t i = 0; i < layers.Size(); i++) {
        const auto& aosLayer = layers[i];
        auto&       pbLayer  = incomingMessage.SMIncomingMessage.run_instances.layers[i];

        utils::StringFromCStr(pbLayer.layer_id) = aosLayer.mLayerID;
        utils::StringFromCStr(pbLayer.digest)   = aosLayer.mLayerDigest;
        utils::StringFromCStr(pbLayer.version)  = aosLayer.mVersion;
        utils::StringFromCStr(pbLayer.url)      = aosLayer.mURL;
        utils::ByteArrayToPB(aosLayer.mSHA256, pbLayer.sha256);
        pbLayer.size = aosLayer.mSize;
    }

    pbRunInstances.instances_count = instances.Size();

    for (size_t i = 0; i < instances.Size(); i++) {
        const auto& aosInstance = instances[i];
        auto&       pbInstance  = incomingMessage.SMIncomingMessage.run_instances.instances[i];

        pbInstance.has_instance = true;
        utils::InstanceIdentToPB(aosInstance.mInstanceIdent, pbInstance.instance);
        pbInstance.uid                                 = aosInstance.mUID;
        pbInstance.priority                            = aosInstance.mPriority;
        utils::StringFromCStr(pbInstance.storage_path) = aosInstance.mStoragePath;
        utils::StringFromCStr(pbInstance.state_path)   = aosInstance.mStatePath;
    }

    err = SendSMIncomingMessage(channel, incomingMessage);
    zassert_true(err.IsNone(), "Error sending message: %s", utils::ErrorToCStr(err));

    // Check run instances

    err = fixture->mLauncher.WaitEvent(cWaitTimeout);
    zassert_true(err.IsNone(), "Error waiting run instances: %s", utils::ErrorToCStr(err));

    auto receivedServices = fixture->mLauncher.GetServices();

    zassert_equal(fixture->mLauncher.IsForceRestart(), incomingMessage.SMIncomingMessage.run_instances.force_restart);
    zassert_equal(fixture->mLauncher.GetServices(), services);
    zassert_equal(fixture->mLauncher.GetLayers(), layers);
    zassert_equal(fixture->mLauncher.GetInstances(), instances);
}

ZTEST_F(smclient, test_RunInstancesStatus)
{
    auto [channel, err] = InitSMClient(fixture, CONFIG_AOS_SM_SECURE_PORT,
        {.mNodeID = "nodeID", .mNodeType = "nodeType", .mStatus = aos::NodeStatusEnum::eProvisioned});
    zassert_true(err.IsNone(), "Getting channel error: %s", utils::ErrorToCStr(err));

    // Send run instances status

    aos::InstanceStatusStaticArray sendRunStatus;

    GenerateInstancesRunStatus(sendRunStatus);

    err = fixture->mSMClient->InstancesRunStatus(sendRunStatus);
    zassert_true(err.IsNone(), "Error sending run instances status: %s", utils::ErrorToCStr(err));

    // Receive run instances status

    servicemanager_v4_SMOutgoingMessages outgoingMessage;
    auto&                                pbRunInstancesStatus = outgoingMessage.SMOutgoingMessage.run_instances_status;

    pbRunInstancesStatus = servicemanager_v4_RunInstancesStatus servicemanager_v4_RunInstancesStatus_init_default;

    err = ReceiveSMOutgoingMessage(channel, outgoingMessage);
    zassert_true(err.IsNone(), "Error receiving message: %s", utils::ErrorToCStr(err));

    zassert_equal(outgoingMessage.which_SMOutgoingMessage,
        servicemanager_v4_SMOutgoingMessages_run_instances_status_tag, "Unexpected message type");

    aos::InstanceStatusStaticArray receivedRunStatus;

    receivedRunStatus.Resize(pbRunInstancesStatus.instances_count);

    for (auto i = 0; i < pbRunInstancesStatus.instances_count; i++) {
        PBToInstanceStatus(pbRunInstancesStatus.instances[i], receivedRunStatus[i]);
    }

    zassert_equal(receivedRunStatus, sendRunStatus, "Wrong run instances status");
}

ZTEST_F(smclient, test_UpdateInstancesStatus)
{
    auto [channel, err] = InitSMClient(fixture, CONFIG_AOS_SM_SECURE_PORT,
        {.mNodeID = "nodeID", .mNodeType = "nodeType", .mStatus = aos::NodeStatusEnum::eProvisioned});
    zassert_true(err.IsNone(), "Getting channel error: %s", utils::ErrorToCStr(err));

    // Send update instances status

    aos::InstanceStatusStaticArray sendUpdateStatus;

    GenerateInstancesRunStatus(sendUpdateStatus);

    err = fixture->mSMClient->InstancesUpdateStatus(sendUpdateStatus);
    zassert_true(err.IsNone(), "Error sending update instances status: %s", utils::ErrorToCStr(err));

    // Receive run instances status

    servicemanager_v4_SMOutgoingMessages outgoingMessage;
    auto& pbUpdateInstancesStatus = outgoingMessage.SMOutgoingMessage.update_instances_status;

    pbUpdateInstancesStatus
        = servicemanager_v4_UpdateInstancesStatus servicemanager_v4_UpdateInstancesStatus_init_default;

    err = ReceiveSMOutgoingMessage(channel, outgoingMessage);
    zassert_true(err.IsNone(), "Error receiving message: %s", utils::ErrorToCStr(err));

    zassert_equal(outgoingMessage.which_SMOutgoingMessage,
        servicemanager_v4_SMOutgoingMessages_update_instances_status_tag, "Unexpected message type");

    aos::InstanceStatusStaticArray receivedUpdateStatus;

    receivedUpdateStatus.Resize(pbUpdateInstancesStatus.instances_count);

    for (auto i = 0; i < outgoingMessage.SMOutgoingMessage.run_instances_status.instances_count; i++) {
        PBToInstanceStatus(pbUpdateInstancesStatus.instances[i], receivedUpdateStatus[i]);
    }

    zassert_equal(receivedUpdateStatus, sendUpdateStatus, "Wrong update instances status");
}

ZTEST_F(smclient, test_ImageContentRequest)
{
    auto [channel, err] = InitSMClient(fixture, CONFIG_AOS_SM_SECURE_PORT,
        {.mNodeID = "nodeID", .mNodeType = "nodeType", .mStatus = aos::NodeStatusEnum::eProvisioned});
    zassert_true(err.IsNone(), "Getting channel error: %s", utils::ErrorToCStr(err));

    // Send image content request

    downloader::ImageContentRequest sendContentRequest {"content URL", 42, aos::DownloadContentEnum::eService};

    err = fixture->mSMClient->SendImageContentRequest(sendContentRequest);
    zassert_true(err.IsNone(), "Error sending image content request: %s", utils::ErrorToCStr(err));

    // Receive image content request

    servicemanager_v4_SMOutgoingMessages outgoingMessage;
    auto& pbImageContentRequest = outgoingMessage.SMOutgoingMessage.image_content_request;

    pbImageContentRequest = servicemanager_v4_ImageContentRequest servicemanager_v4_ImageContentRequest_init_default;

    err = ReceiveSMOutgoingMessage(channel, outgoingMessage);
    zassert_true(err.IsNone(), "Error receiving message: %s", utils::ErrorToCStr(err));

    zassert_equal(outgoingMessage.which_SMOutgoingMessage,
        servicemanager_v4_SMOutgoingMessages_image_content_request_tag, "Unexpected message type");

    downloader::ImageContentRequest receiveContentRequest;

    receiveContentRequest.mRequestID = pbImageContentRequest.request_id;
    utils::PBToEnum(pbImageContentRequest.content_type, receiveContentRequest.mContentType);
    receiveContentRequest.mURL = utils::StringFromCStr(pbImageContentRequest.url);

    zassert_equal(receiveContentRequest, sendContentRequest);
}

ZTEST_F(smclient, test_ImageContentInfo)
{
    auto [channel, err] = InitSMClient(fixture, CONFIG_AOS_SM_SECURE_PORT,
        {.mNodeID = "nodeID", .mNodeType = "nodeType", .mStatus = aos::NodeStatusEnum::eProvisioned});
    zassert_true(err.IsNone(), "Getting channel error: %s", utils::ErrorToCStr(err));

    // Send image content info

    downloader::ImageContentInfo aosContentInfo;

    GenerateImageContentInfo(aosContentInfo);

    servicemanager_v4_SMIncomingMessages incomingMessage;
    auto&                                pbImageContentInfo = incomingMessage.SMIncomingMessage.image_content_info;

    incomingMessage.which_SMIncomingMessage = servicemanager_v4_SMIncomingMessages_image_content_info_tag;
    pbImageContentInfo = servicemanager_v4_ImageContentInfo servicemanager_v4_ImageContentInfo_init_default;

    pbImageContentInfo.request_id        = aosContentInfo.mRequestID;
    pbImageContentInfo.image_files_count = aosContentInfo.mFiles.Size();

    for (size_t i = 0; i < aosContentInfo.mFiles.Size(); i++) {
        const auto& aosFileInfo = aosContentInfo.mFiles[i];
        auto&       pbFileInfo  = incomingMessage.SMIncomingMessage.image_content_info.image_files[i];

        utils::StringFromCStr(pbFileInfo.relative_path) = aosFileInfo.mRelativePath;
        utils::ByteArrayToPB(aosFileInfo.mSHA256, pbFileInfo.sha256);
        pbFileInfo.size = aosFileInfo.mSize;
    }

    if (!aosContentInfo.mError.IsNone()) {
        pbImageContentInfo.has_error = true;
        pbImageContentInfo.error     = utils::ErrorToPB(aosContentInfo.mError);
    }

    err = SendSMIncomingMessage(channel, incomingMessage);
    zassert_true(err.IsNone(), "Error sending message: %s", utils::ErrorToCStr(err));

    // Receive image content info

    err = fixture->mDownloader.WaitEvent(cWaitTimeout);
    zassert_true(err.IsNone(), "Error waiting downloader event: %s", utils::ErrorToCStr(err));

    zassert_equal(fixture->mDownloader.GetContentInfo(), aosContentInfo);
}

ZTEST_F(smclient, test_ImageContent)
{
    auto [channel, err] = InitSMClient(fixture, CONFIG_AOS_SM_SECURE_PORT,
        {.mNodeID = "nodeID", .mNodeType = "nodeType", .mStatus = aos::NodeStatusEnum::eProvisioned});
    zassert_true(err.IsNone(), "Getting channel error: %s", utils::ErrorToCStr(err));

    // Send image content

    uint8_t data[]       = {12, 23, 45, 32, 21, 56, 12, 18};
    auto    aosFileChunk = downloader::FileChunk {43, "path/to/file1", 4, 1, aos::Array<uint8_t>(data, sizeof(data))};

    servicemanager_v4_SMIncomingMessages incomingMessage;
    auto&                                pbImageContent = incomingMessage.SMIncomingMessage.image_content;

    incomingMessage.which_SMIncomingMessage = servicemanager_v4_SMIncomingMessages_image_content_tag;
    pbImageContent = servicemanager_v4_ImageContent servicemanager_v4_ImageContent_init_default;

    pbImageContent.request_id                           = aosFileChunk.mRequestID;
    utils::StringFromCStr(pbImageContent.relative_path) = aosFileChunk.mRelativePath;
    pbImageContent.parts_count                          = aosFileChunk.mPartsCount;
    pbImageContent.part                                 = aosFileChunk.mPart;
    utils::ByteArrayToPB(aosFileChunk.mData, pbImageContent.data);

    err = SendSMIncomingMessage(channel, incomingMessage);
    zassert_true(err.IsNone(), "Error sending message: %s", utils::ErrorToCStr(err));

    // Receive image content

    err = fixture->mDownloader.WaitEvent(cWaitTimeout);
    zassert_true(err.IsNone(), "Error waiting downloader event: %s", utils::ErrorToCStr(err));

    zassert_equal(fixture->mDownloader.GetFileChunk(), aosFileChunk);
}

ZTEST_F(smclient, test_ConnectionNotification)
{
    ConnectionSubscriberStub subscriber;

    auto err = fixture->mSMClient->Subscribe(subscriber);
    zassert_true(err.IsNone(), "Error subscribing connection notification: %s", utils::ErrorToCStr(err));

    ChannelStub* channel = nullptr;

    aos::Tie(channel, err) = InitSMClient(fixture, CONFIG_AOS_SM_SECURE_PORT,
        {.mNodeID = "nodeID", .mNodeType = "nodeType", .mStatus = aos::NodeStatusEnum::eProvisioned});
    zassert_true(err.IsNone(), "Getting channel error: %s", utils::ErrorToCStr(err));

    // Wait connected

    err = subscriber.WaitConnect(cWaitTimeout);
    zassert_true(err.IsNone(), "Wait connection error: %s", err.Message());

    // Wait disconnected

    fixture->mSMClient->OnClockUnsynced();

    err = subscriber.WaitDisconnect(cWaitTimeout);
    zassert_true(err.IsNone(), "Wait connection error: %s", err.Message());

    fixture->mSMClient->Unsubscribe(subscriber);
}
