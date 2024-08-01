/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/tc_util.h>
#include <zephyr/ztest.h>

#include "communication/communication.hpp"
#include "utils/pbconvert.hpp"

#include "stubs/certhandlerstub.hpp"
#include "stubs/clocksyncstub.hpp"
#include "stubs/commchannelstub.hpp"
#include "stubs/connectionsubscriberstub.hpp"
#include "stubs/downloaderstub.hpp"
#include "stubs/launcherstub.hpp"
#include "stubs/monitoringstub.hpp"
#include "stubs/provisioningstub.hpp"
#include "stubs/resourcemanagerstub.hpp"
#include "utils.hpp"

/***********************************************************************************************************************
 * Types
 **********************************************************************************************************************/

struct cmclient_fixture {
    CommChannelStub     mOpenChannel;
    CommChannelStub     mSecureChannel;
    LauncherStub        mLauncher;
    CertHandlerStub     mCertHandler;
    ResourceManagerStub mResourceManager;
    ResourceMonitorStub mResourceMonitor;
    DownloaderStub      mDownloader;
    ClockSyncStub       mClockSync;
    ProvisioningStub    mProvisioning;
    Communication       mCommunication;
};

/***********************************************************************************************************************
 * Static
 **********************************************************************************************************************/

static constexpr void PBToInstanceStatus(
    const servicemanager_v3_InstanceStatus& pbInstance, aos::InstanceStatus& aosInstance)
{
    if (pbInstance.has_instance) {
        PBToString(pbInstance.instance.service_id, aosInstance.mInstanceIdent.mServiceID);
        PBToString(pbInstance.instance.subject_id, aosInstance.mInstanceIdent.mSubjectID);
        aosInstance.mInstanceIdent.mInstance = pbInstance.instance.instance;
    }

    aosInstance.mAosVersion = pbInstance.aos_version;
    PBToEnum(pbInstance.run_state, aosInstance.mRunState);

    if (pbInstance.has_error_info) {
        if (pbInstance.error_info.exit_code) {
            aosInstance.mError = pbInstance.error_info.exit_code;
        } else {
            aosInstance.mError = static_cast<aos::ErrorEnum>(pbInstance.error_info.aos_code);
        }
    }
}

static void PBToMonitoringData(
    const servicemanager_v3_MonitoringData& pbMonitoring, aos::monitoring::MonitoringData& aosMonitoring)
{
    aosMonitoring.mRAM = pbMonitoring.ram;
    aosMonitoring.mCPU = pbMonitoring.cpu;
    aosMonitoring.mDisk.Resize(pbMonitoring.disk_count);

    for (size_t i = 0; i < pbMonitoring.disk_count; i++) {

        PBToString(pbMonitoring.disk[i].name, aosMonitoring.mDisk[i].mName);
        aosMonitoring.mDisk[i].mUsedSize = pbMonitoring.disk[i].used_size;
    }

    aosMonitoring.mInTraffic  = pbMonitoring.in_traffic;
    aosMonitoring.mOutTraffic = pbMonitoring.out_traffic;
}

static aos::Error SendCMIncomingMessage(CommChannelStub& channel, const servicemanager_v3_SMIncomingMessages& message)
{
    return SendPBMessage(channel, AOS_VCHAN_SM, "", 0, &servicemanager_v3_SMIncomingMessages_msg, &message,
        servicemanager_v3_SMOutgoingMessages_size);
}

static aos::Error ReceiveCMOutgoingMessage(CommChannelStub& channel, servicemanager_v3_SMOutgoingMessages& message)
{
    return ReceivePBMessage(channel, AOS_VCHAN_SM, "", 0, &servicemanager_v3_SMOutgoingMessages_msg, &message,
        servicemanager_v3_SMOutgoingMessages_size);
}

/***********************************************************************************************************************
 * Setup
 **********************************************************************************************************************/

ZTEST_SUITE(
    cmclient, nullptr,
    []() -> void* {
        aos::Log::SetCallback(TestLogCallback);

        auto fixture = new cmclient_fixture;

        fixture->mProvisioning.SetProvisioned(true);

        auto err = fixture->mCommunication.Init(fixture->mOpenChannel, fixture->mSecureChannel, fixture->mLauncher,
            fixture->mCertHandler, fixture->mResourceManager, fixture->mResourceMonitor, fixture->mDownloader,
            fixture->mClockSync, fixture->mProvisioning);
        zassert_true(err.IsNone(), "Can't initialize communication: %s", err.Message());

        fixture->mCommunication.ClockSynced();

        // Wait node configuration message

        servicemanager_v3_SMOutgoingMessages outgoingMessage servicemanager_v3_SMOutgoingMessages_init_default;

        err = ReceiveCMOutgoingMessage(fixture->mSecureChannel, outgoingMessage);
        zassert_true(err.IsNone(), "Error receiving message: %s", err.Message());

        zassert_equal(
            outgoingMessage.which_SMOutgoingMessage, servicemanager_v3_SMOutgoingMessages_node_configuration_tag);

        return fixture;
    },
    [](void* fixture) {
        auto f = static_cast<cmclient_fixture*>(fixture);

        f->mOpenChannel.Clear();
        f->mSecureChannel.Clear();
        f->mLauncher.Clear();
        f->mCertHandler.Clear();
        f->mResourceManager.Clear();
        f->mResourceMonitor.Clear();
        f->mDownloader.Clear();
        f->mClockSync.Clear();
        f->mProvisioning.SetProvisioned(true);
    },
    nullptr, [](void* fixture) { delete static_cast<cmclient_fixture*>(fixture); });

/***********************************************************************************************************************
 * Tests
 **********************************************************************************************************************/

ZTEST_F(cmclient, test_NodeConfiguration)
{
    ConnectionSubscriberStub subscriber;

    fixture->mCommunication.Subscribes(subscriber);

    aos::NodeInfo sendNodeInfo {CONFIG_AOS_NODE_ID, 2, 1024, {}};

    sendNodeInfo.mPartitions.EmplaceBack(aos::monitoring::PartitionInfo {"var", "", {}, 2048, 0});
    sendNodeInfo.mPartitions.end()->mTypes.EmplaceBack("data");
    sendNodeInfo.mPartitions.end()->mTypes.EmplaceBack("state");
    sendNodeInfo.mPartitions.end()->mTypes.EmplaceBack("storage");
    sendNodeInfo.mPartitions.EmplaceBack(aos::monitoring::PartitionInfo {"aos", "", {}, 4096, 0});
    sendNodeInfo.mPartitions.end()->mTypes.EmplaceBack("aos");
    sendNodeInfo.mPartitions.end()->mTypes.EmplaceBack("tmp");

    fixture->mResourceMonitor.SetNodeInfo(sendNodeInfo);

    fixture->mSecureChannel.SendRead(std::vector<uint8_t>(), aos::ErrorEnum::eFailed);

    auto err = subscriber.WaitDisconnect(cWaitTimeout);
    zassert_true(err.IsNone(), "Wait connection error: %s", err.Message());

    err = subscriber.WaitConnect(cWaitTimeout);
    zassert_true(err.IsNone(), "Wait connection error: %s", err.Message());

    // Wait node configuration message

    servicemanager_v3_SMOutgoingMessages outgoingMessage servicemanager_v3_SMOutgoingMessages_init_default;

    err = ReceiveCMOutgoingMessage(fixture->mSecureChannel, outgoingMessage);
    zassert_true(err.IsNone(), "Error receiving message: %s", err.Message());

    aos::NodeInfo receiveNodeInfo {};
    const auto&   pbNodeConfiguration = outgoingMessage.SMOutgoingMessage.node_configuration;

    PBToString(pbNodeConfiguration.node_id, receiveNodeInfo.mNodeID);
    receiveNodeInfo.mNumCPUs  = pbNodeConfiguration.num_cpus;
    receiveNodeInfo.mTotalRAM = pbNodeConfiguration.total_ram;
    receiveNodeInfo.mPartitions.Resize(pbNodeConfiguration.partitions_count);

    for (size_t i = 0; i < pbNodeConfiguration.partitions_count; ++i) {
        PBToString(pbNodeConfiguration.partitions[i].name, receiveNodeInfo.mPartitions[i].mName);
        receiveNodeInfo.mPartitions[i].mTotalSize = pbNodeConfiguration.partitions[i].total_size;
        receiveNodeInfo.mPartitions[i].mTypes.Resize(pbNodeConfiguration.partitions[i].types_count);

        for (size_t j = 0; j < pbNodeConfiguration.partitions[i].types_count; ++j) {
            PBToString(pbNodeConfiguration.partitions[i].types[j], receiveNodeInfo.mPartitions[i].mTypes[j]);
        }
    }

    zassert_equal(outgoingMessage.which_SMOutgoingMessage, servicemanager_v3_SMOutgoingMessages_node_configuration_tag);
    zassert_equal(strcmp(pbNodeConfiguration.node_type, CONFIG_AOS_NODE_TYPE), 0);
    zassert_equal(pbNodeConfiguration.remote_node, true);
    zassert_equal(pbNodeConfiguration.runner_features_count, 1);
    zassert_equal(strcmp(pbNodeConfiguration.runner_features[0], "xrun"), 0);
    zassert_equal(receiveNodeInfo, sendNodeInfo);

    fixture->mCommunication.Unsubscribes(subscriber);
}

ZTEST_F(cmclient, test_GetUnitConfigStatus)
{
    auto version    = "1.1.1";
    auto unitConfig = "unitConfig";

    fixture->mResourceManager.UpdateUnitConfig(version, unitConfig);
    fixture->mResourceManager.SetError(aos::ErrorEnum::eFailed);

    servicemanager_v3_SMIncomingMessages incomingMessage servicemanager_v3_SMIncomingMessages_init_default;

    incomingMessage.which_SMIncomingMessage = servicemanager_v3_SMIncomingMessages_get_unit_config_status_tag;

    auto err = SendCMIncomingMessage(fixture->mSecureChannel, incomingMessage);
    zassert_true(err.IsNone(), "Error sending message: %s", err.Message());

    servicemanager_v3_SMOutgoingMessages outgoingMessage servicemanager_v3_SMOutgoingMessages_init_default;

    err = ReceiveCMOutgoingMessage(fixture->mSecureChannel, outgoingMessage);
    zassert_true(err.IsNone(), "Error receiving message: %s", err.Message());

    zassert_equal(outgoingMessage.which_SMOutgoingMessage, servicemanager_v3_SMOutgoingMessages_unit_config_status_tag);
    zassert_equal(strcmp(outgoingMessage.SMOutgoingMessage.unit_config_status.vendor_version, version), 0);
    zassert_not_equal(strstr(outgoingMessage.SMOutgoingMessage.unit_config_status.error,
                          aos::Error(aos::ErrorEnum::eFailed).Message()),
        nullptr);
}

ZTEST_F(cmclient, test_CheckUnitConfig)
{
    auto                                                 version    = "1.2.0";
    auto                                                 unitConfig = "unitConfig";
    servicemanager_v3_SMIncomingMessages incomingMessage servicemanager_v3_SMIncomingMessages_init_default;

    incomingMessage.which_SMIncomingMessage = servicemanager_v3_SMIncomingMessages_check_unit_config_tag;
    strcpy(incomingMessage.SMIncomingMessage.check_unit_config.vendor_version, version);
    strcpy(incomingMessage.SMIncomingMessage.check_unit_config.unit_config, unitConfig);

    auto err = SendCMIncomingMessage(fixture->mSecureChannel, incomingMessage);
    zassert_true(err.IsNone(), "Error sending message: %s", err.Message());

    servicemanager_v3_SMOutgoingMessages outgoingMessage servicemanager_v3_SMOutgoingMessages_init_default;

    err = ReceiveCMOutgoingMessage(fixture->mSecureChannel, outgoingMessage);
    zassert_true(err.IsNone(), "Error receiving message: %s", err.Message());

    zassert_equal(outgoingMessage.which_SMOutgoingMessage, servicemanager_v3_SMOutgoingMessages_unit_config_status_tag);
    zassert_equal(strcmp(outgoingMessage.SMOutgoingMessage.unit_config_status.vendor_version, version), 0);
    zassert_equal(strlen(outgoingMessage.SMOutgoingMessage.unit_config_status.error), 0);
}

ZTEST_F(cmclient, test_SetUnitConfig)
{
    auto                                                 version    = "1.2.0";
    auto                                                 unitConfig = "unitConfig";
    servicemanager_v3_SMIncomingMessages incomingMessage servicemanager_v3_SMIncomingMessages_init_default;

    incomingMessage.which_SMIncomingMessage = servicemanager_v3_SMIncomingMessages_set_unit_config_tag;
    strcpy(incomingMessage.SMIncomingMessage.set_unit_config.vendor_version, version);
    strcpy(incomingMessage.SMIncomingMessage.set_unit_config.unit_config, unitConfig);

    auto err = SendCMIncomingMessage(fixture->mSecureChannel, incomingMessage);
    zassert_true(err.IsNone(), "Error sending message: %s", err.Message());

    servicemanager_v3_SMOutgoingMessages outgoingMessage servicemanager_v3_SMOutgoingMessages_init_default;

    err = ReceiveCMOutgoingMessage(fixture->mSecureChannel, outgoingMessage);
    zassert_true(err.IsNone(), "Error receiving message: %s", err.Message());

    zassert_equal(outgoingMessage.which_SMOutgoingMessage, servicemanager_v3_SMOutgoingMessages_unit_config_status_tag);
    zassert_equal(strcmp(outgoingMessage.SMOutgoingMessage.unit_config_status.vendor_version, version), 0);
    zassert_equal(strlen(outgoingMessage.SMOutgoingMessage.unit_config_status.error), 0);
    zassert_equal(fixture->mResourceManager.GetUnitConfig(), unitConfig);

    auto ret = fixture->mResourceManager.GetVersion();
    zassert_true(ret.mError.IsNone(), "Failed to get version: %s", ret.mError.Message());
    zassert_equal(ret.mValue, version);
}

ZTEST_F(cmclient, test_RunInstances)
{
    servicemanager_v3_SMIncomingMessages incomingMessage servicemanager_v3_SMIncomingMessages_init_default;

    incomingMessage.which_SMIncomingMessage = servicemanager_v3_SMIncomingMessages_run_instances_tag;
    incomingMessage.SMIncomingMessage.run_instances.force_restart = true;

    uint8_t sha256[] = {1, 2, 3, 4, 5, 6, 7, 8};
    uint8_t sha512[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};

    // Fill services

    aos::ServiceInfoStaticArray services;

    services.EmplaceBack(aos::ServiceInfo {{0, "1.0.0", "this is service 1"}, "service1", "provider1", 42,
        "service1 URL", aos::Array<uint8_t>(sha256, sizeof(sha256)), aos::Array<uint8_t>(sha512, sizeof(sha512)), 312});
    services.EmplaceBack(aos::ServiceInfo {{1, "1.0.1", "this is service 2"}, "service2", "provider2", 43,
        "service2 URL", aos::Array<uint8_t>(sha256, sizeof(sha256)), aos::Array<uint8_t>(sha512, sizeof(sha512)), 512});

    incomingMessage.SMIncomingMessage.run_instances.services_count = services.Size();

    for (size_t i = 0; i < services.Size(); i++) {
        const auto& aosService = services[i];
        auto&       pbService  = incomingMessage.SMIncomingMessage.run_instances.services[i];

        pbService.has_version_info = true;
        VersionInfoToPB(aosService.mVersionInfo, pbService.version_info);
        StringToPB(aosService.mServiceID, pbService.service_id);
        StringToPB(aosService.mProviderID, pbService.provider_id);
        pbService.gid = aosService.mGID;
        StringToPB(aosService.mURL, pbService.url);
        ByteArrayToPB(aosService.mSHA256, pbService.sha256);
        ByteArrayToPB(aosService.mSHA512, pbService.sha512);
        pbService.size = aosService.mSize;
    }

    // Fill layers

    aos::LayerInfoStaticArray layers;

    layers.EmplaceBack(aos::LayerInfo {{2, "2.1.0", "this is layer 1"}, "layer1", "digest1", "layer1 URL",
        aos::Array<uint8_t>(sha256, sizeof(sha256)), aos::Array<uint8_t>(sha512, sizeof(sha512)), 1024});
    layers.EmplaceBack(aos::LayerInfo {{3, "2.2.0", "this is layer 2"}, "layer2", "digest2", "layer2 URL",
        aos::Array<uint8_t>(sha256, sizeof(sha256)), aos::Array<uint8_t>(sha512, sizeof(sha512)), 2048});
    layers.EmplaceBack(aos::LayerInfo {{4, "2.3.0", "this is layer 3"}, "layer3", "digest3", "layer3 URL",
        aos::Array<uint8_t>(sha256, sizeof(sha256)), aos::Array<uint8_t>(sha512, sizeof(sha512)), 4096});

    incomingMessage.SMIncomingMessage.run_instances.layers_count = layers.Size();

    for (size_t i = 0; i < layers.Size(); i++) {
        const auto& aosLayer = layers[i];
        auto&       pbLayer  = incomingMessage.SMIncomingMessage.run_instances.layers[i];

        pbLayer.has_version_info = true;
        VersionInfoToPB(aosLayer.mVersionInfo, pbLayer.version_info);
        StringToPB(aosLayer.mLayerID, pbLayer.layer_id);
        StringToPB(aosLayer.mLayerDigest, pbLayer.digest);
        StringToPB(aosLayer.mURL, pbLayer.url);
        ByteArrayToPB(aosLayer.mSHA256, pbLayer.sha256);
        ByteArrayToPB(aosLayer.mSHA512, pbLayer.sha512);
        pbLayer.size = aosLayer.mSize;
    }

    // Fill instances

    aos::InstanceInfoStaticArray instances;

    instances.EmplaceBack(aos::InstanceInfo {{"service1", "subject1", 1}, 1, 1, "storage1", "state1"});
    instances.EmplaceBack(aos::InstanceInfo {{"service2", "subject2", 2}, 2, 2, "storage2", "state2"});
    instances.EmplaceBack(aos::InstanceInfo {{"service3", "subject3", 3}, 3, 3, "storage3", "state3"});
    instances.EmplaceBack(aos::InstanceInfo {{"service4", "subject4", 4}, 4, 4, "storage4", "state4"});

    incomingMessage.SMIncomingMessage.run_instances.instances_count = instances.Size();

    for (size_t i = 0; i < instances.Size(); i++) {
        const auto& aosInstance = instances[i];
        auto&       pbInstance  = incomingMessage.SMIncomingMessage.run_instances.instances[i];

        pbInstance.has_instance = true;
        InstanceIdentToPB(aosInstance.mInstanceIdent, pbInstance.instance);
        pbInstance.uid      = aosInstance.mUID;
        pbInstance.priority = aosInstance.mPriority;
        StringToPB(aosInstance.mStoragePath, pbInstance.storage_path);
        StringToPB(aosInstance.mStatePath, pbInstance.state_path);
    }

    // Send message and check result

    auto err = SendCMIncomingMessage(fixture->mSecureChannel, incomingMessage);
    zassert_true(err.IsNone(), "Error sending message: %s", err.Message());

    err = fixture->mLauncher.WaitEvent(cWaitTimeout);
    zassert_true(err.IsNone(), "Error waiting run instances: %s", err.Message());

    zassert_equal(fixture->mLauncher.IsForceRestart(), incomingMessage.SMIncomingMessage.run_instances.force_restart);
    zassert_equal(fixture->mLauncher.GetServices(), services);
    zassert_equal(fixture->mLauncher.GetLayers(), layers);
    zassert_equal(fixture->mLauncher.GetInstances(), instances);
}

ZTEST_F(cmclient, test_ImageContentInfo)
{
    auto                                                 aosContentInfo = ImageContentInfo {42};
    servicemanager_v3_SMIncomingMessages incomingMessage servicemanager_v3_SMIncomingMessages_init_default;
    uint8_t                                              sha256[] = {1, 2, 3, 4, 5, 6, 7, 8};

    aosContentInfo.mFiles.EmplaceBack(FileInfo {"path/to/file1", aos::Array<uint8_t>(sha256, sizeof(sha256)), 1024});
    aosContentInfo.mFiles.EmplaceBack(FileInfo {"path/to/file2", aos::Array<uint8_t>(sha256, sizeof(sha256)), 2048});
    aosContentInfo.mFiles.EmplaceBack(FileInfo {"path/to/file3", aos::Array<uint8_t>(sha256, sizeof(sha256)), 4096});
    aosContentInfo.mError = "content info error";

    incomingMessage.which_SMIncomingMessage = servicemanager_v3_SMIncomingMessages_image_content_info_tag;
    incomingMessage.SMIncomingMessage.image_content_info.request_id        = aosContentInfo.mRequestID;
    incomingMessage.SMIncomingMessage.image_content_info.image_files_count = aosContentInfo.mFiles.Size();

    for (size_t i = 0; i < aosContentInfo.mFiles.Size(); i++) {
        const auto& aosFileInfo = aosContentInfo.mFiles[i];
        auto&       pbFileInfo  = incomingMessage.SMIncomingMessage.image_content_info.image_files[i];

        StringToPB(aosFileInfo.mRelativePath, pbFileInfo.relative_path);
        ByteArrayToPB(aosFileInfo.mSHA256, pbFileInfo.sha256);
        pbFileInfo.size = aosFileInfo.mSize;
    }

    StringToPB(aosContentInfo.mError, incomingMessage.SMIncomingMessage.image_content_info.error);

    auto err = SendCMIncomingMessage(fixture->mSecureChannel, incomingMessage);
    zassert_true(err.IsNone(), "Error sending message: %s", err.Message());

    err = fixture->mDownloader.WaitEvent(cWaitTimeout);
    zassert_true(err.IsNone(), "Error waiting downloader event: %s", err.Message());

    zassert_equal(fixture->mDownloader.GetContentInfo(), aosContentInfo);
}

ZTEST_F(cmclient, test_ImageContent)
{
    uint8_t data[]       = {12, 23, 45, 32, 21, 56, 12, 18};
    auto    aosFileChunk = FileChunk {43, "path/to/file1", 4, 1, aos::Array<uint8_t>(data, sizeof(data))};
    servicemanager_v3_SMIncomingMessages incomingMessage servicemanager_v3_SMIncomingMessages_init_default;

    incomingMessage.which_SMIncomingMessage                    = servicemanager_v3_SMIncomingMessages_image_content_tag;
    incomingMessage.SMIncomingMessage.image_content.request_id = aosFileChunk.mRequestID;
    StringToPB(aosFileChunk.mRelativePath, incomingMessage.SMIncomingMessage.image_content.relative_path);
    incomingMessage.SMIncomingMessage.image_content.parts_count = aosFileChunk.mPartsCount;
    incomingMessage.SMIncomingMessage.image_content.part        = aosFileChunk.mPart;
    ByteArrayToPB(aosFileChunk.mData, incomingMessage.SMIncomingMessage.image_content.data);

    auto err = SendCMIncomingMessage(fixture->mSecureChannel, incomingMessage);
    zassert_true(err.IsNone(), "Error sending message: %s", err.Message());

    err = fixture->mDownloader.WaitEvent(cWaitTimeout);
    zassert_true(err.IsNone(), "Error waiting downloader event: %s", err.Message());

    zassert_equal(fixture->mDownloader.GetFileChunk(), aosFileChunk);
}

ZTEST_F(cmclient, test_InstancesRunStatus)
{
    aos::InstanceStatusStaticArray sendRunStatus {};

    sendRunStatus.EmplaceBack(aos::InstanceStatus {
        {"service1", "subject1", 0}, 4, aos::InstanceRunStateEnum::eActive, aos::ErrorEnum::eNone});
    sendRunStatus.EmplaceBack(
        aos::InstanceStatus {{"service1", "subject1", 1}, 4, aos::InstanceRunStateEnum::eFailed, 42});
    sendRunStatus.EmplaceBack(aos::InstanceStatus {
        {"service1", "subject1", 2}, 4, aos::InstanceRunStateEnum::eFailed, aos::ErrorEnum::eRuntime});

    auto err = fixture->mCommunication.InstancesRunStatus(sendRunStatus);
    zassert_true(err.IsNone(), "Error sending run instances status: %s", err.Message());

    servicemanager_v3_SMOutgoingMessages outgoingMessage servicemanager_v3_SMOutgoingMessages_init_default;

    err = ReceiveCMOutgoingMessage(fixture->mSecureChannel, outgoingMessage);
    zassert_true(err.IsNone(), "Error receiving message: %s", err.Message());

    aos::InstanceStatusStaticArray receiveRunStatus {};

    for (auto i = 0; i < outgoingMessage.SMOutgoingMessage.run_instances_status.instances_count; i++) {
        aos::InstanceStatus instanceStatus;

        PBToInstanceStatus(outgoingMessage.SMOutgoingMessage.run_instances_status.instances[i], instanceStatus);
        receiveRunStatus.PushBack(instanceStatus);
    }

    zassert_equal(
        outgoingMessage.which_SMOutgoingMessage, servicemanager_v3_SMOutgoingMessages_run_instances_status_tag);
    zassert_equal(receiveRunStatus, sendRunStatus);
}

ZTEST_F(cmclient, test_InstancesUpdateStatus)
{
    aos::InstanceStatusStaticArray sendUpdateStatus {};

    sendUpdateStatus.EmplaceBack(aos::InstanceStatus {
        {"service1", "subject1", 0}, 1, aos::InstanceRunStateEnum::eFailed, aos::ErrorEnum::eNoMemory});
    sendUpdateStatus.EmplaceBack(aos::InstanceStatus {
        {"service1", "subject1", 1}, 1, aos::InstanceRunStateEnum::eActive, aos::ErrorEnum::eNone});

    auto err = fixture->mCommunication.InstancesUpdateStatus(sendUpdateStatus);
    zassert_true(err.IsNone(), "Error sending update instances status: %s", err.Message());

    servicemanager_v3_SMOutgoingMessages outgoingMessage servicemanager_v3_SMOutgoingMessages_init_default;

    err = ReceiveCMOutgoingMessage(fixture->mSecureChannel, outgoingMessage);
    zassert_true(err.IsNone(), "Error receiving message: %s", err.Message());

    aos::InstanceStatusStaticArray receiveUpdateStatus {};

    for (auto i = 0; i < outgoingMessage.SMOutgoingMessage.update_instances_status.instances_count; i++) {
        aos::InstanceStatus instanceStatus;

        PBToInstanceStatus(outgoingMessage.SMOutgoingMessage.update_instances_status.instances[i], instanceStatus);
        receiveUpdateStatus.PushBack(instanceStatus);
    }

    zassert_equal(
        outgoingMessage.which_SMOutgoingMessage, servicemanager_v3_SMOutgoingMessages_update_instances_status_tag);
    zassert_equal(receiveUpdateStatus, sendUpdateStatus);
}

ZTEST_F(cmclient, test_ImageContentRequest)
{
    ImageContentRequest sendContentRequest {"content URL", 42, aos::DownloadContentEnum::eService};

    auto err = fixture->mCommunication.SendImageContentRequest(sendContentRequest);
    zassert_true(err.IsNone(), "Error sending image content request: %s", err.Message());

    servicemanager_v3_SMOutgoingMessages outgoingMessage servicemanager_v3_SMOutgoingMessages_init_default;

    err = ReceiveCMOutgoingMessage(fixture->mSecureChannel, outgoingMessage);
    zassert_true(err.IsNone(), "Error receiving message: %s", err.Message());

    ImageContentRequest receiveContentRequest {};

    PBToString(outgoingMessage.SMOutgoingMessage.image_content_request.url, receiveContentRequest.mURL);
    receiveContentRequest.mRequestID = outgoingMessage.SMOutgoingMessage.image_content_request.request_id;
    PBToEnum(outgoingMessage.SMOutgoingMessage.image_content_request.content_type, receiveContentRequest.mContentType);

    zassert_equal(
        outgoingMessage.which_SMOutgoingMessage, servicemanager_v3_SMOutgoingMessages_image_content_request_tag);
    zassert_equal(receiveContentRequest, sendContentRequest);
}

ZTEST_F(cmclient, test_MonitoringData)
{
    aos::monitoring::NodeMonitoringData sendMonitoringData {"", {1024, 100, {}, 10, 20}, {10, 20}};

    sendMonitoringData.mMonitoringData.mDisk.EmplaceBack(aos::monitoring::PartitionInfo {"disk1", "", {}, 0, 2048});
    sendMonitoringData.mMonitoringData.mDisk.EmplaceBack(aos::monitoring::PartitionInfo {"disk2", "", {}, 0, 4096});

    sendMonitoringData.mServiceInstances.EmplaceBack(
        aos::monitoring::InstanceMonitoringData {"", {"service1", "subject1", 0}, {512, 50, {}, 30, 30}});
    sendMonitoringData.mServiceInstances.end()->mMonitoringData.mDisk.EmplaceBack(
        aos::monitoring::PartitionInfo {"disk3", "", {}, 0, 192});
    sendMonitoringData.mServiceInstances.end()->mMonitoringData.mDisk.EmplaceBack(
        aos::monitoring::PartitionInfo {"disk4", "", {}, 0, 243});
    sendMonitoringData.mServiceInstances.EmplaceBack(
        aos::monitoring::InstanceMonitoringData {"", {"service1", "subject1", 1}, {256, 32, {}, 33, 32}});
    sendMonitoringData.mServiceInstances.end()->mMonitoringData.mDisk.EmplaceBack(
        aos::monitoring::PartitionInfo {"disk5", "", {}, 0, 543});
    sendMonitoringData.mServiceInstances.EmplaceBack(
        aos::monitoring::InstanceMonitoringData {"", {"service1", "subject1", 2}, {128, 15, {}, 43, 12}});

    auto err = fixture->mCommunication.SendMonitoringData(sendMonitoringData);
    zassert_true(err.IsNone(), "Error sending monitoring data: %s", err.Message());

    servicemanager_v3_SMOutgoingMessages outgoingMessage servicemanager_v3_SMOutgoingMessages_init_default;

    err = ReceiveCMOutgoingMessage(fixture->mSecureChannel, outgoingMessage);
    zassert_true(err.IsNone(), "Error receiving message: %s", err.Message());

    aos::monitoring::NodeMonitoringData receiveMonitoringData {};

    const auto& pbMonitoringData = outgoingMessage.SMOutgoingMessage.node_monitoring;

    if (pbMonitoringData.has_monitoring_data) {
        PBToMonitoringData(pbMonitoringData.monitoring_data, receiveMonitoringData.mMonitoringData);
    }

    receiveMonitoringData.mServiceInstances.Resize(pbMonitoringData.instance_monitoring_count);

    for (size_t i = 0; i < pbMonitoringData.instance_monitoring_count; i++) {
        if (pbMonitoringData.instance_monitoring[i].has_instance) {
            PBToInstanceIdent(pbMonitoringData.instance_monitoring[i].instance,
                receiveMonitoringData.mServiceInstances[i].mInstanceIdent);
        }

        if (pbMonitoringData.instance_monitoring[i].has_monitoring_data) {
            PBToMonitoringData(pbMonitoringData.instance_monitoring[i].monitoring_data,
                receiveMonitoringData.mServiceInstances[i].mMonitoringData);
        }
    }

    if (pbMonitoringData.has_timestamp) {
        receiveMonitoringData.mTimestamp.tv_sec  = pbMonitoringData.timestamp.seconds;
        receiveMonitoringData.mTimestamp.tv_nsec = pbMonitoringData.timestamp.nanos;
    }

    zassert_equal(outgoingMessage.which_SMOutgoingMessage, servicemanager_v3_SMOutgoingMessages_node_monitoring_tag);
    zassert_equal(receiveMonitoringData, sendMonitoringData);
}

ZTEST_F(cmclient, test_ClockSync)
{
    fixture->mCommunication.ClockUnsynced();

    fixture->mSecureChannel.SendRead(std::vector<uint8_t>(), aos::ErrorEnum::eFailed);

    // Wait clock sync start

    auto err = fixture->mClockSync.WaitEvent(cWaitTimeout);
    zassert_true(err.IsNone(), "Error waiting clock sync start: %s", err.Message());

    zassert_true(fixture->mClockSync.GetStarted());

    // Wait clock sync request

    err = fixture->mCommunication.SendClockSyncRequest();
    zassert_true(err.IsNone(), "Error sending clock sync request: %s", err.Message());

    servicemanager_v3_SMOutgoingMessages outgoingMessage servicemanager_v3_SMOutgoingMessages_init_default;

    err = ReceiveCMOutgoingMessage(fixture->mOpenChannel, outgoingMessage);
    zassert_true(err.IsNone(), "Error receiving message: %s", err.Message());

    zassert_equal(outgoingMessage.which_SMOutgoingMessage, servicemanager_v3_SMOutgoingMessages_clock_sync_request_tag);

    // Send clock sync

    servicemanager_v3_SMIncomingMessages incomingMessage servicemanager_v3_SMIncomingMessages_init_default;

    incomingMessage.which_SMIncomingMessage                       = servicemanager_v3_SMIncomingMessages_clock_sync_tag;
    incomingMessage.SMIncomingMessage.clock_sync.has_current_time = true;
    incomingMessage.SMIncomingMessage.clock_sync.current_time.seconds = 43;
    incomingMessage.SMIncomingMessage.clock_sync.current_time.nanos   = 234;

    err = SendCMIncomingMessage(fixture->mOpenChannel, incomingMessage);
    zassert_true(err.IsNone(), "Error sending message: %s", err.Message());

    err = fixture->mClockSync.WaitEvent(cWaitTimeout);
    zassert_true(err.IsNone(), "Error waiting clock sync start: %s", err.Message());

    zassert_equal(fixture->mClockSync.GetSyncTime(),
        aos::Time::Unix(incomingMessage.SMIncomingMessage.clock_sync.current_time.seconds,
            incomingMessage.SMIncomingMessage.clock_sync.current_time.nanos));

    fixture->mCommunication.ClockSynced();

    // Wait node configuration

    outgoingMessage = servicemanager_v3_SMOutgoingMessages servicemanager_v3_SMOutgoingMessages_init_default;

    err = ReceiveCMOutgoingMessage(fixture->mSecureChannel, outgoingMessage);
    zassert_true(err.IsNone(), "Error receiving message: %s", err.Message());

    zassert_equal(outgoingMessage.which_SMOutgoingMessage, servicemanager_v3_SMOutgoingMessages_node_configuration_tag);
}
