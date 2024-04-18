/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <aos/common/tools/memory.hpp>

#include <pb_decode.h>

#include "utils/pbconvert.hpp"

#include "cmclient.hpp"
#include "log.hpp"

/***********************************************************************************************************************
 * Static
 **********************************************************************************************************************/

static void InstanceStatusToPB(const aos::InstanceStatus& aosInstance, servicemanager_v3_InstanceStatus& pbInstance)
{
    pbInstance.has_instance = true;
    StringToPB(aosInstance.mInstanceIdent.mServiceID, pbInstance.instance.service_id);
    StringToPB(aosInstance.mInstanceIdent.mSubjectID, pbInstance.instance.subject_id);
    pbInstance.instance.instance = aosInstance.mInstanceIdent.mInstance;
    pbInstance.aos_version       = aosInstance.mAosVersion;
    StringToPB(aosInstance.mRunState.ToString(), pbInstance.run_state);

    if (!aosInstance.mError.IsNone()) {
        pbInstance.has_error_info       = true;
        pbInstance.error_info.aos_code  = static_cast<int32_t>(aosInstance.mError.Value());
        pbInstance.error_info.exit_code = aosInstance.mError.Errno();
        ErrorToPB(aosInstance.mError, pbInstance.error_info.message);
    }
}

static void MonitoringDataToPB(
    const aos::monitoring::MonitoringData& aosMonitoring, servicemanager_v3_MonitoringData& pbMonitoring)
{
    pbMonitoring.ram        = aosMonitoring.mRAM;
    pbMonitoring.cpu        = aosMonitoring.mCPU;
    pbMonitoring.disk_count = aosMonitoring.mDisk.Size();

    for (size_t i = 0; i < aosMonitoring.mDisk.Size(); i++) {
        StringToPB(aosMonitoring.mDisk[i].mName, pbMonitoring.disk[i].name);
        pbMonitoring.disk[i].used_size = aosMonitoring.mDisk[i].mUsedSize;
    }

    pbMonitoring.in_traffic  = aosMonitoring.mInTraffic;
    pbMonitoring.out_traffic = aosMonitoring.mOutTraffic;
}

/***********************************************************************************************************************
 * Public
 **********************************************************************************************************************/

aos::Error CMClient::Init(aos::sm::launcher::LauncherItf& launcher,
    aos::sm::resourcemanager::ResourceManagerItf& resourceManager, aos::monitoring::ResourceMonitorItf& resourceMonitor,
    DownloadReceiverItf& downloader, ClockSyncItf& clockSync, MessageSenderItf& messageSender)
{
    LOG_DBG() << "Initialize CM client";

    mLauncher        = &launcher;
    mResourceManager = &resourceManager;
    mResourceMonitor = &resourceMonitor;
    mDownloader      = &downloader;
    mClockSync       = &clockSync;

    auto err = MessageHandler::Init(AOS_VCHAN_SM, messageSender);
    if (!err.IsNone()) {
        return err;
    }

    if (!(err = mSMService.Init("")).IsNone()) {
        return err;
    }

    mSMService.RegisterMethod("");

    if (!(err = RegisterService(mSMService, ChannelEnum::eOpen)).IsNone()) {
        return err;
    }

    if (!(err = RegisterService(mSMService, ChannelEnum::eSecure)).IsNone()) {
        return err;
    }

    return aos::ErrorEnum::eNone;
}

aos::Error CMClient::InstancesRunStatus(const aos::Array<aos::InstanceStatus>& instances)
{
    auto  outgoingMessage = aos::MakeUnique<servicemanager_v3_SMOutgoingMessages>(&mAllocator);
    auto& pbRunStatus     = outgoingMessage->SMOutgoingMessage.run_instances_status;

    outgoingMessage->which_SMOutgoingMessage = servicemanager_v3_SMOutgoingMessages_run_instances_status_tag;
    pbRunStatus = servicemanager_v3_RunInstancesStatus servicemanager_v3_RunInstancesStatus_init_default;

    outgoingMessage->SMOutgoingMessage.run_instances_status.instances_count = instances.Size();

    for (size_t i = 0; i < instances.Size(); i++) {
        InstanceStatusToPB(instances[i], pbRunStatus.instances[i]);
    }

    LOG_DBG() << "Send SM message: message = RunInstancesStatus";

    return SendPBMessage(ChannelEnum::eSecure, 0, &servicemanager_v3_SMOutgoingMessages_msg, outgoingMessage.Get());
}

aos::Error CMClient::InstancesUpdateStatus(const aos::Array<aos::InstanceStatus>& instances)
{
    auto  outgoingMessage = aos::MakeUnique<servicemanager_v3_SMOutgoingMessages>(&mAllocator);
    auto& pbUpdateStatus  = outgoingMessage->SMOutgoingMessage.update_instances_status;

    outgoingMessage->which_SMOutgoingMessage = servicemanager_v3_SMOutgoingMessages_update_instances_status_tag;
    pbUpdateStatus = servicemanager_v3_UpdateInstancesStatus servicemanager_v3_UpdateInstancesStatus_init_default;

    outgoingMessage->SMOutgoingMessage.update_instances_status.instances_count = instances.Size();

    for (size_t i = 0; i < instances.Size(); i++) {
        InstanceStatusToPB(instances[i], pbUpdateStatus.instances[i]);
    }

    LOG_DBG() << "Send SM message: message = UpdateInstancesStatus";

    return SendPBMessage(ChannelEnum::eSecure, 0, &servicemanager_v3_SMOutgoingMessages_msg, outgoingMessage.Get());
}

aos::Error CMClient::SendImageContentRequest(const ImageContentRequest& request)
{
    auto  outgoingMessage  = aos::MakeUnique<servicemanager_v3_SMOutgoingMessages>(&mAllocator);
    auto& pbContentRequest = outgoingMessage->SMOutgoingMessage.image_content_request;

    outgoingMessage->which_SMOutgoingMessage = servicemanager_v3_SMOutgoingMessages_image_content_request_tag;
    pbContentRequest = servicemanager_v3_ImageContentRequest servicemanager_v3_ImageContentRequest_init_default;

    StringToPB(request.mURL, pbContentRequest.url);
    pbContentRequest.request_id = request.mRequestID;
    StringToPB(request.mContentType.ToString(), pbContentRequest.content_type);

    LOG_DBG() << "Send SM message: message = ImageContentRequest";

    return SendPBMessage(ChannelEnum::eSecure, 0, &servicemanager_v3_SMOutgoingMessages_msg, outgoingMessage.Get());
}

aos::Error CMClient::SendMonitoringData(const aos::monitoring::NodeMonitoringData& monitoringData)
{
    auto  outgoingMessage  = aos::MakeUnique<servicemanager_v3_SMOutgoingMessages>(&mAllocator);
    auto& pbMonitoringData = outgoingMessage->SMOutgoingMessage.node_monitoring;

    outgoingMessage->which_SMOutgoingMessage = servicemanager_v3_SMOutgoingMessages_node_monitoring_tag;
    pbMonitoringData = servicemanager_v3_NodeMonitoring servicemanager_v3_NodeMonitoring_init_default;

    pbMonitoringData.has_monitoring_data = true;
    MonitoringDataToPB(monitoringData.mMonitoringData, pbMonitoringData.monitoring_data);

    pbMonitoringData.instance_monitoring_count = monitoringData.mServiceInstances.Size();
    for (size_t i = 0; i < monitoringData.mServiceInstances.Size(); i++) {
        pbMonitoringData.instance_monitoring[i].has_instance = true;
        InstanceIdentToPB(
            monitoringData.mServiceInstances[i].mInstanceIdent, pbMonitoringData.instance_monitoring[i].instance);

        pbMonitoringData.instance_monitoring[i].has_monitoring_data = true;
        MonitoringDataToPB(monitoringData.mServiceInstances[i].mMonitoringData,
            pbMonitoringData.instance_monitoring[i].monitoring_data);
    }

    pbMonitoringData.has_timestamp     = true;
    pbMonitoringData.timestamp.seconds = monitoringData.mTimestamp.tv_sec;
    pbMonitoringData.timestamp.nanos   = monitoringData.mTimestamp.tv_nsec;

    LOG_DBG() << "Send SM message: message = NodeMonitoring";

    return SendPBMessage(ChannelEnum::eSecure, 0, &servicemanager_v3_SMOutgoingMessages_msg, outgoingMessage.Get());
}

aos::Error CMClient::SendNodeConfiguration()
{
    auto  nodeInfo            = aos::MakeUnique<aos::monitoring::NodeInfo>(&mAllocator);
    auto  outgoingMessage     = aos::MakeUnique<servicemanager_v3_SMOutgoingMessages>(&mAllocator);
    auto& pbNodeConfiguration = outgoingMessage->SMOutgoingMessage.node_configuration;

    outgoingMessage->which_SMOutgoingMessage = servicemanager_v3_SMOutgoingMessages_node_configuration_tag;
    pbNodeConfiguration = servicemanager_v3_NodeConfiguration servicemanager_v3_NodeConfiguration_init_default;

    auto err = mResourceMonitor->GetNodeInfo(*nodeInfo);
    if (!err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    StringToPB(cNodeID, pbNodeConfiguration.node_id);
    StringToPB(cNodeType, pbNodeConfiguration.node_type);
    pbNodeConfiguration.remote_node           = true;
    pbNodeConfiguration.runner_features_count = 1;
    StringToPB(cRunner, pbNodeConfiguration.runner_features[0]);
    pbNodeConfiguration.num_cpus         = nodeInfo->mNumCPUs;
    pbNodeConfiguration.total_ram        = nodeInfo->mTotalRAM;
    pbNodeConfiguration.partitions_count = nodeInfo->mPartitions.Size();

    for (size_t i = 0; i < nodeInfo->mPartitions.Size(); ++i) {
        StringToPB(nodeInfo->mPartitions[i].mName, pbNodeConfiguration.partitions[i].name);
        pbNodeConfiguration.partitions[i].total_size  = nodeInfo->mPartitions[i].mTotalSize;
        pbNodeConfiguration.partitions[i].types_count = nodeInfo->mPartitions[i].mTypes.Size();

        for (size_t j = 0; j < nodeInfo->mPartitions[i].mTypes.Size(); ++j) {
            StringToPB(nodeInfo->mPartitions[i].mTypes[j], pbNodeConfiguration.partitions[i].types[j]);
        }
    }

    LOG_DBG() << "Send SM message: message = NodeConfiguration";

    return SendPBMessage(ChannelEnum::eSecure, 0, &servicemanager_v3_SMOutgoingMessages_msg, outgoingMessage.Get());
}

aos::Error CMClient::SendClockSyncRequest()
{
    auto  outgoingMessage    = aos::MakeUnique<servicemanager_v3_SMOutgoingMessages>(&mAllocator);
    auto& pbClockSyncRequest = outgoingMessage->SMOutgoingMessage.clock_sync_request;

    outgoingMessage->which_SMOutgoingMessage = servicemanager_v3_SMOutgoingMessages_clock_sync_request_tag;
    pbClockSyncRequest = servicemanager_v3_ClockSyncRequest servicemanager_v3_ClockSyncRequest_init_default;

    LOG_DBG() << "Send SM message: message = ClockSyncRequest";

    return SendPBMessage(ChannelEnum::eOpen, 0, &servicemanager_v3_SMOutgoingMessages_msg, outgoingMessage.Get());
}

/***********************************************************************************************************************
 * Private
 **********************************************************************************************************************/

aos::Error CMClient::ProcessMessage(Channel channel, PBServiceItf& service, const aos::String& methodName,
    uint64_t requestID, const aos::Array<uint8_t>& data)
{
    auto message = aos::MakeUnique<servicemanager_v3_SMIncomingMessages>(&mAllocator);
    auto stream  = pb_istream_from_buffer(data.Get(), data.Size());

    auto status = pb_decode(&stream, &servicemanager_v3_SMIncomingMessages_msg, message.Get());
    if (!status) {
        LOG_ERR() << "Can't decode incoming message: " << PB_GET_ERROR(&stream);
        return AOS_ERROR_WRAP(aos::ErrorEnum::eRuntime);
    }

    aos::Error err;

    if (channel == ChannelEnum::eOpen) {
        switch (message->which_SMIncomingMessage) {
        case servicemanager_v3_SMIncomingMessages_clock_sync_tag:
            err = ProcessClockSync(message->SMIncomingMessage.clock_sync);
            break;

        default:
            LOG_WRN() << "Receive unsupported message tag: " << message->which_SMIncomingMessage;
            break;
        }
    }

    if (channel == ChannelEnum::eSecure) {
        switch (message->which_SMIncomingMessage) {
        case servicemanager_v3_SMIncomingMessages_get_unit_config_status_tag:
            err = ProcessGetUnitConfigStatus(channel, requestID);
            break;

        case servicemanager_v3_SMIncomingMessages_check_unit_config_tag:
            err = ProcessCheckUnitConfig(channel, requestID, message->SMIncomingMessage.check_unit_config);
            break;

        case servicemanager_v3_SMIncomingMessages_set_unit_config_tag:
            err = ProcessSetUnitConfig(channel, requestID, message->SMIncomingMessage.set_unit_config);
            break;

        case servicemanager_v3_SMIncomingMessages_run_instances_tag:
            err = ProcessRunInstances(message->SMIncomingMessage.run_instances);
            break;

        case servicemanager_v3_SMIncomingMessages_image_content_info_tag:
            err = ProcessImageContentInfo(message->SMIncomingMessage.image_content_info);
            break;

        case servicemanager_v3_SMIncomingMessages_image_content_tag:
            err = ProcessImageContent(message->SMIncomingMessage.image_content);
            break;

        default:
            LOG_WRN() << "Receive unsupported message tag: " << message->which_SMIncomingMessage;
            break;
        }
    }

    return err;
}

aos::Error CMClient::ProcessGetUnitConfigStatus(Channel channel, uint64_t requestID)
{
    LOG_DBG() << "Receive SM message: message = GetUnitConfigStatus";

    auto  outgoingMessage = aos::MakeUnique<servicemanager_v3_SMOutgoingMessages>(&mAllocator);
    auto& pbConfigStatus  = outgoingMessage->SMOutgoingMessage.unit_config_status;

    outgoingMessage->which_SMOutgoingMessage = servicemanager_v3_SMOutgoingMessages_unit_config_status_tag;
    pbConfigStatus                           = servicemanager_v3_UnitConfigStatus_init_default;

    aos::Error                          err;
    aos::StaticString<aos::cVersionLen> version;

    aos::Tie(version, err) = mResourceManager->GetVersion();
    if (!err.IsNone()) {
        err = AOS_ERROR_WRAP(err);
        ErrorToPB(err, pbConfigStatus.error);
    }

    StringToPB(version, pbConfigStatus.vendor_version);

    LOG_DBG() << "Send SM message: message = UnitConfigStatus";

    auto sendErr = SendPBMessage(channel, requestID, &servicemanager_v3_SMOutgoingMessages_msg, outgoingMessage.Get());
    if (!sendErr.IsNone() && err.IsNone()) {
        err = sendErr;
    }

    return err;
}

aos::Error CMClient::ProcessCheckUnitConfig(
    Channel channel, uint64_t requestID, const servicemanager_v3_CheckUnitConfig& pbUnitConfig)
{
    LOG_DBG() << "Receive SM message: message = CheckUnitConfig";

    auto  outgoingMessage = aos::MakeUnique<servicemanager_v3_SMOutgoingMessages>(&mAllocator);
    auto& pbConfigStatus  = outgoingMessage->SMOutgoingMessage.unit_config_status;

    outgoingMessage->which_SMOutgoingMessage = servicemanager_v3_SMOutgoingMessages_unit_config_status_tag;
    pbConfigStatus                           = servicemanager_v3_UnitConfigStatus_init_default;

    auto err = mResourceManager->CheckUnitConfig(pbUnitConfig.vendor_version, pbUnitConfig.unit_config);
    if (!err.IsNone()) {
        err = AOS_ERROR_WRAP(err);
        ErrorToPB(err, pbConfigStatus.error);
    }

    aos::StaticString<aos::cVersionLen> version;

    PBToString(pbUnitConfig.vendor_version, version);
    StringToPB(version, pbConfigStatus.vendor_version);

    LOG_DBG() << "Send SM message: message = UnitConfigStatus";

    auto sendErr = SendPBMessage(channel, requestID, &servicemanager_v3_SMOutgoingMessages_msg, outgoingMessage.Get());
    if (!sendErr.IsNone() && err.IsNone()) {
        err = sendErr;
    }

    return err;
}

aos::Error CMClient::ProcessSetUnitConfig(
    Channel channel, uint64_t requestID, const servicemanager_v3_SetUnitConfig& pbUnitConfig)
{
    LOG_DBG() << "Receive SM message: message = SetUnitConfig";

    auto  outgoingMessage = aos::MakeUnique<servicemanager_v3_SMOutgoingMessages>(&mAllocator);
    auto& pbConfigStatus  = outgoingMessage->SMOutgoingMessage.unit_config_status;

    outgoingMessage->which_SMOutgoingMessage = servicemanager_v3_SMOutgoingMessages_unit_config_status_tag;
    pbConfigStatus                           = servicemanager_v3_UnitConfigStatus_init_default;

    auto err = mResourceManager->UpdateUnitConfig(pbUnitConfig.vendor_version, pbUnitConfig.unit_config);
    if (!err.IsNone()) {
        err = AOS_ERROR_WRAP(err);
        ErrorToPB(err, pbConfigStatus.error);
    }

    strncpy(pbConfigStatus.vendor_version, pbUnitConfig.vendor_version, sizeof(pbConfigStatus.vendor_version));

    LOG_DBG() << "Send SM message: message = UnitConfigStatus";

    auto sendErr = SendPBMessage(channel, requestID, &servicemanager_v3_SMOutgoingMessages_msg, outgoingMessage.Get());
    if (!sendErr.IsNone() && err.IsNone()) {
        err = sendErr;
    }

    return err;
}

aos::Error CMClient::ProcessRunInstances(const servicemanager_v3_RunInstances& pbRunInstances)
{
    LOG_DBG() << "Receive SM message: message = RunInstances";

    auto aosServices = aos::MakeUnique<aos::ServiceInfoStaticArray>(&mAllocator);

    aosServices->Resize(pbRunInstances.services_count);

    for (auto i = 0; i < pbRunInstances.services_count; i++) {
        const auto& pbService  = pbRunInstances.services[i];
        auto&       aosService = (*aosServices)[i];

        if (pbService.has_version_info) {
            PBToVersionInfo(pbService.version_info, aosService.mVersionInfo);
        }

        PBToString(pbService.service_id, aosService.mServiceID);
        PBToString(pbService.provider_id, aosService.mProviderID);
        aosService.mGID = pbService.gid;
        aosService.mURL = pbService.url;
        PBToByteArray(pbService.sha256, aosService.mSHA256);
        PBToByteArray(pbService.sha512, aosService.mSHA512);
        aosService.mSize = pbService.size;
    }

    auto aosLayers = aos::MakeUnique<aos::LayerInfoStaticArray>(&mAllocator);

    aosLayers->Resize(pbRunInstances.layers_count);

    for (auto i = 0; i < pbRunInstances.layers_count; i++) {
        const auto& pbLayer  = pbRunInstances.layers[i];
        auto&       aosLayer = (*aosLayers)[i];

        if (pbLayer.has_version_info) {
            PBToVersionInfo(pbLayer.version_info, aosLayer.mVersionInfo);
        }

        PBToString(pbLayer.layer_id, aosLayer.mLayerID);
        PBToString(pbLayer.digest, aosLayer.mLayerDigest);
        PBToString(pbLayer.url, aosLayer.mURL);
        PBToByteArray(pbLayer.sha256, aosLayer.mSHA256);
        PBToByteArray(pbLayer.sha512, aosLayer.mSHA512);
        aosLayer.mSize = pbLayer.size;
    }

    auto aosInstances = aos::MakeUnique<aos::InstanceInfoStaticArray>(&mAllocator);

    aosInstances->Resize(pbRunInstances.instances_count);

    for (auto i = 0; i < pbRunInstances.instances_count; i++) {
        const auto& pbInstance  = pbRunInstances.instances[i];
        auto&       aosInstance = (*aosInstances)[i];

        if (pbInstance.has_instance) {
            PBToString(pbInstance.instance.service_id, aosInstance.mInstanceIdent.mServiceID);
            PBToString(pbInstance.instance.subject_id, aosInstance.mInstanceIdent.mSubjectID);
            aosInstance.mInstanceIdent.mInstance = pbInstance.instance.instance;
        }

        aosInstance.mUID      = pbInstance.uid;
        aosInstance.mPriority = pbInstance.priority;
        PBToString(pbInstance.storage_path, aosInstance.mStoragePath);
        PBToString(pbInstance.state_path, aosInstance.mStatePath);
    }

    auto err = mLauncher->RunInstances(*aosServices, *aosLayers, *aosInstances, pbRunInstances.force_restart);
    if (!err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    return aos::ErrorEnum::eNone;
}

aos::Error CMClient::ProcessImageContentInfo(const servicemanager_v3_ImageContentInfo& pbContentInfo)
{
    LOG_DBG() << "Receive SM message: message = ImageContentInfo";

    auto aosContentInfo = aos::MakeUnique<ImageContentInfo>(&mAllocator);

    aosContentInfo->mRequestID = pbContentInfo.request_id;

    for (auto i = 0; i < pbContentInfo.image_files_count; i++) {
        FileInfo fileInfo;

        PBToString(pbContentInfo.image_files[i].relative_path, fileInfo.mRelativePath);
        PBToByteArray(pbContentInfo.image_files[i].sha256, fileInfo.mSHA256);
        fileInfo.mSize = pbContentInfo.image_files[i].size;

        aosContentInfo->mFiles.PushBack(fileInfo);
    }

    aosContentInfo->mError = pbContentInfo.error;

    auto err = mDownloader->ReceiveImageContentInfo(*aosContentInfo);
    if (!err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    return aos::ErrorEnum::eNone;
}

aos::Error CMClient::ProcessImageContent(const servicemanager_v3_ImageContent& pbContent)
{
    LOG_DBG() << "Receive SM message: message = ImageContent";

    auto chunk = aos::MakeUnique<FileChunk>(&mAllocator);

    chunk->mRequestID = pbContent.request_id;
    PBToString(pbContent.relative_path, chunk->mRelativePath);
    chunk->mPartsCount = pbContent.parts_count;
    chunk->mPart       = pbContent.part;
    PBToByteArray(pbContent.data, chunk->mData);

    auto err = mDownloader->ReceiveFileChunk(*chunk);
    if (!err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    return aos::ErrorEnum::eNone;
}

aos::Error CMClient::ProcessClockSync(const servicemanager_v3_ClockSync& pbClockSync)
{
    LOG_DBG() << "Receive SM message: message = ClockSync";

    if (!pbClockSync.has_current_time) {
        return aos::ErrorEnum::eInvalidArgument;
    }

    auto err = mClockSync->Sync(aos::Time::Unix(pbClockSync.current_time.seconds, pbClockSync.current_time.nanos));
    if (!err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    return aos::ErrorEnum::eNone;
}
