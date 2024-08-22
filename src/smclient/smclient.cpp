/*
 * Copyright (C) 2024 Renesas Electronics Corporation.
 * Copyright (C) 2024 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <pb_decode.h>

#include "utils/pbconvert.hpp"

#include "log.hpp"
#include "smclient.hpp"

#include "communication/pbhandler.cpp"

namespace aos::zephyr::smclient {

/***********************************************************************************************************************
 * Static
 **********************************************************************************************************************/

static google_protobuf_Timestamp TimestampToPB(const Time& time)
{
    auto unixTime = time.UnixTime();

    return google_protobuf_Timestamp {unixTime.tv_sec, static_cast<int32_t>(unixTime.tv_nsec)};
}

static void MonitoringDataToPB(const monitoring::MonitoringData& monitoringData, const Time& timestamp,
    servicemanager_v4_MonitoringData& pbMonitoringData)
{
    pbMonitoringData.cpu           = monitoringData.mCPU;
    pbMonitoringData.ram           = monitoringData.mRAM;
    pbMonitoringData.download      = monitoringData.mDownload;
    pbMonitoringData.upload        = monitoringData.mUpload;
    pbMonitoringData.disk_count    = monitoringData.mDisk.Size();
    pbMonitoringData.has_timestamp = true;
    pbMonitoringData.timestamp     = TimestampToPB(timestamp);

    for (size_t i = 0; i < monitoringData.mDisk.Size(); i++) {
        utils::StringFromCStr(pbMonitoringData.disk[i].name) = monitoringData.mDisk[i].mName;
        pbMonitoringData.disk[i].used_size                   = monitoringData.mDisk[i].mUsedSize;
    }
}

template <typename T>
static constexpr void NodeMonitoringToPB(const monitoring::NodeMonitoringData& nodeMonitoring, T& pbNodeMonitoring)
{
    pbNodeMonitoring.has_node_monitoring = true;
    MonitoringDataToPB(nodeMonitoring.mMonitoringData, nodeMonitoring.mTimestamp, pbNodeMonitoring.node_monitoring);
    pbNodeMonitoring.instances_monitoring_count = nodeMonitoring.mServiceInstances.Size();

    for (size_t i = 0; i < nodeMonitoring.mServiceInstances.Size(); i++) {
        pbNodeMonitoring.instances_monitoring[i].has_instance = true;
        utils::InstanceIdentToPB(
            nodeMonitoring.mServiceInstances[i].mInstanceIdent, pbNodeMonitoring.instances_monitoring[i].instance);
        pbNodeMonitoring.instances_monitoring[i].has_monitoring_data = true;
        MonitoringDataToPB(nodeMonitoring.mServiceInstances[i].mMonitoringData, nodeMonitoring.mTimestamp,
            pbNodeMonitoring.instances_monitoring[i].monitoring_data);
    }
}

/***********************************************************************************************************************
 * Public
 **********************************************************************************************************************/

Error SMClient::Init(iam::nodeinfoprovider::NodeInfoProviderItf& nodeInfoProvider,
    sm::resourcemanager::ResourceManagerItf& resourceManager, monitoring::ResourceMonitorItf& resourceMonitor,
    clocksync::ClockSyncItf& clockSync, communication::ChannelManagerItf& channelManager)
{
    LOG_DBG() << "Initialize SM client";

    mNodeInfoProvider = &nodeInfoProvider;
    mResourceManager  = &resourceManager;
    mResourceMonitor  = &resourceMonitor;
    mChannelManager   = &channelManager;

    auto nodeInfo = MakeUnique<NodeInfo>(&mAllocator);

    if (auto err = mNodeInfoProvider->GetNodeInfo(*nodeInfo); !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    mProvisioned = nodeInfo->mStatus != NodeStatusEnum::eUnprovisioned;

    auto [openChannel, err] = mChannelManager->CreateChannel(cOpenPort);
    if (!err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    if (err = mOpenHandler.Init(*openChannel, clockSync); !err.IsNone()) {
        return err;
    }

    if (err = clockSync.Subscribe(*this); err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    if (err = mNodeInfoProvider->SubscribeNodeStatusChanged(*this); err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    return ErrorEnum::eNone;
}

SMClient::~SMClient()
{
    if (IsStarted()) {
        if (auto err = Stop(); !err.IsNone()) {
            LOG_ERR() << "Failed to stop PB handler: err=" << err;
        }

        if (auto err = mChannelManager->DeleteChannel(cSecurePort); !err.IsNone()) {
            LOG_ERR() << "Failed to delete channel: err=" << err;
        }
    }

    if (mOpenHandler.IsStarted()) {
        if (auto err = mOpenHandler.Stop(); !err.IsNone()) {
            LOG_ERR() << "Failed to stop open handler: err=" << err;
        }

        if (auto err = mChannelManager->DeleteChannel(cOpenPort); !err.IsNone()) {
            LOG_ERR() << "Failed to delete channel: err=" << err;
        }
    }
}

Error SMClient::InstancesRunStatus(const Array<InstanceStatus>& instances)
{
    return ErrorEnum::eNone;
}

Error SMClient::InstancesUpdateStatus(const Array<InstanceStatus>& instances)
{
    return ErrorEnum::eNone;
}

Error SMClient::SendImageContentRequest(const downloader::ImageContentRequest& request)
{
    return ErrorEnum::eNone;
}

Error SMClient::SendMonitoringData(const monitoring::NodeMonitoringData& nodeMonitoring)
{
    LockGuard lock {mMutex};

    LOG_INF() << "Send node monitoring";

    auto  outgoingMessage     = aos::MakeUnique<servicemanager_v4_SMOutgoingMessages>(&mAllocator);
    auto& pbInstantMonitoring = outgoingMessage->SMOutgoingMessage.instant_monitoring;

    outgoingMessage->which_SMOutgoingMessage = servicemanager_v4_SMOutgoingMessages_instant_monitoring_tag;
    pbInstantMonitoring = servicemanager_v4_InstantMonitoring servicemanager_v4_InstantMonitoring_init_default;

    NodeMonitoringToPB(nodeMonitoring, pbInstantMonitoring);

    return SendMessage(outgoingMessage.Get(), &servicemanager_v4_SMOutgoingMessages_msg);
}

Error SMClient::Subscribes(ConnectionSubscriberItf& subscriber)
{
    return ErrorEnum::eNone;
}

void SMClient::Unsubscribes(ConnectionSubscriberItf& subscriber)
{
}

Error SMClient::OnNodeStatusChanged(const String& nodeID, const NodeStatus& status)
{
    LOG_DBG() << "Node status changed: status=" << status;

    {
        LockGuard lock {mMutex};

        mProvisioned = status != NodeStatusEnum::eUnprovisioned;
    }

    UpdatePBHandlerState();

    return ErrorEnum::eNone;
}

void SMClient::OnClockSynced()
{
    LOG_DBG() << "Clock synced";

    {
        LockGuard lock {mMutex};

        mClockSynced = true;
    }

    UpdatePBHandlerState();
}

void SMClient::OnClockUnsynced()
{
    LOG_DBG() << "Clock unsynced";

    {
        LockGuard lock {mMutex};

        mClockSynced = false;
    }

    UpdatePBHandlerState();
}

Error SMClient::SendClockSyncRequest()
{
    return mOpenHandler.SendClockSyncRequest();
}

/***********************************************************************************************************************
 * Private
 **********************************************************************************************************************/

void SMClient::OnConnect()
{
}

void SMClient::OnDisconnect()
{
}

Error SMClient::ReceiveMessage(const Array<uint8_t>& data)
{
    LockGuard lock {mMutex};

    auto incomingMessages = MakeUnique<servicemanager_v4_SMIncomingMessages>(&mAllocator);
    auto stream           = pb_istream_from_buffer(data.Get(), data.Size());

    if (auto status = pb_decode(&stream, &servicemanager_v4_SMIncomingMessages_msg, incomingMessages.Get()); !status) {
        return AOS_ERROR_WRAP(Error(ErrorEnum::eRuntime, "failed to decode message"));
    }

    Error err;

    switch (incomingMessages->which_SMIncomingMessage) {
    case servicemanager_v4_SMIncomingMessages_get_node_config_status_tag:
        err = ProcessGetNodeConfigStatus(incomingMessages->SMIncomingMessage.get_node_config_status);
        break;

    case servicemanager_v4_SMIncomingMessages_check_node_config_tag:
        err = ProcessCheckNodeConfig(incomingMessages->SMIncomingMessage.check_node_config);
        break;

    case servicemanager_v4_SMIncomingMessages_set_node_config_tag:
        err = ProcessSetNodeConfig(incomingMessages->SMIncomingMessage.set_node_config);
        break;

    case servicemanager_v4_SMIncomingMessages_get_average_monitoring_tag:
        err = ProcessGetAverageMonitoring(incomingMessages->SMIncomingMessage.get_average_monitoring);
        break;

    default:
        LOG_WRN() << "Receive unsupported message: tag=" << incomingMessages->which_SMIncomingMessage;
        break;
    }

    return err;
}

Error SMClient::ProcessGetNodeConfigStatus(const servicemanager_v4_GetNodeConfigStatus& pbGetNodeConfigStatus)
{
    LOG_INF() << "Process get node config status";

    auto [version, configErr] = mResourceManager->GetNodeConfigVersion();

    return SendNodeConfigStatus(version, configErr);
}

Error SMClient::ProcessCheckNodeConfig(const servicemanager_v4_CheckNodeConfig& pbCheckNodeConfig)
{
    auto version = utils::StringFromCStr(pbCheckNodeConfig.version);

    LOG_INF() << "Process check node config: version=" << version;

    auto configErr = mResourceManager->CheckNodeConfig(version, utils::StringFromCStr(pbCheckNodeConfig.node_config));

    return SendNodeConfigStatus(version, configErr);
}

Error SMClient::ProcessSetNodeConfig(const servicemanager_v4_SetNodeConfig& pbSetNodeConfig)
{
    auto version = utils::StringFromCStr(pbSetNodeConfig.version);

    LOG_INF() << "Process set node config: version=" << version;

    auto configErr = mResourceManager->UpdateNodeConfig(version, utils::StringFromCStr(pbSetNodeConfig.node_config));

    return SendNodeConfigStatus(version, configErr);
}

Error SMClient::ProcessGetAverageMonitoring(const servicemanager_v4_GetAverageMonitoring& pbGetAverageMonitoring)
{
    LOG_INF() << "Process get average monitoring";

    auto  outgoingMessage     = aos::MakeUnique<servicemanager_v4_SMOutgoingMessages>(&mAllocator);
    auto& pbAverageMonitoring = outgoingMessage->SMOutgoingMessage.average_monitoring;

    outgoingMessage->which_SMOutgoingMessage = servicemanager_v4_SMOutgoingMessages_average_monitoring_tag;
    pbAverageMonitoring = servicemanager_v4_AverageMonitoring servicemanager_v4_AverageMonitoring_init_default;

    auto averageMonitoring = aos::MakeUnique<monitoring::NodeMonitoringData>(&mAllocator);

    if (auto err = mResourceMonitor->GetAverageMonitoringData(*averageMonitoring); !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    NodeMonitoringToPB(*averageMonitoring, pbAverageMonitoring);

    return SendMessage(outgoingMessage.Get(), &servicemanager_v4_SMOutgoingMessages_msg);
}

Error SMClient::SendNodeConfigStatus(const String& version, const Error& configErr)
{
    LOG_INF() << "Send node config status: version=" << version << ", configErr=" << configErr;

    auto  outgoingMessage    = aos::MakeUnique<servicemanager_v4_SMOutgoingMessages>(&mAllocator);
    auto& pbNodeConfigStatus = outgoingMessage->SMOutgoingMessage.node_config_status;

    outgoingMessage->which_SMOutgoingMessage = servicemanager_v4_SMOutgoingMessages_node_config_status_tag;
    pbNodeConfigStatus = servicemanager_v4_NodeConfigStatus servicemanager_v4_NodeConfigStatus_init_default;

    auto nodeInfo = MakeUnique<NodeInfo>(&mAllocator);

    if (auto err = mNodeInfoProvider->GetNodeInfo(*nodeInfo); !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    utils::StringFromCStr(pbNodeConfigStatus.node_id)   = nodeInfo->mNodeID;
    utils::StringFromCStr(pbNodeConfigStatus.node_type) = nodeInfo->mNodeType;
    utils::StringFromCStr(pbNodeConfigStatus.version)   = version;

    if (!configErr.IsNone()) {
        utils::ErrorToPB(configErr, pbNodeConfigStatus);
    }

    return SendMessage(outgoingMessage.Get(), &servicemanager_v4_SMOutgoingMessages_msg);
}

void SMClient::UpdatePBHandlerState()
{
    auto start = false;
    auto stop  = false;

    {
        LockGuard lock {mMutex};

        if (mClockSynced && mProvisioned && !IsStarted()) {
            start = true;
        }

        if ((!mClockSynced || !mProvisioned) && IsStarted()) {
            stop = true;
        }
    }

    if (start) {
        auto [secureChannel, err] = mChannelManager->CreateChannel(cSecurePort);
        if (!err.IsNone()) {
            LOG_ERR() << "Failed to create channel: err=" << err;
            return;
        }

        if (err = PBHandler::Init("SM secure", *secureChannel); !err.IsNone()) {
            LOG_ERR() << "Failed to init PB handler: err=" << err;
            return;
        }

        if (err = Start(); !err.IsNone()) {
            LOG_ERR() << "Failed to start PB handler: err=" << err;
            return;
        }

        auto [version, configErr] = mResourceManager->GetNodeConfigVersion();

        if (err = SendNodeConfigStatus(version, configErr); !err.IsNone()) {
            LOG_ERR() << "Failed to send node config status: err=" << err;
            return;
        }
    }

    if (stop) {
        if (auto err = Stop(); !err.IsNone()) {
            LOG_ERR() << "Failed to stop PB handler: err=" << err;
            return;
        }

        if (auto err = mChannelManager->DeleteChannel(cSecurePort); !err.IsNone()) {
            LOG_ERR() << "Failed to delete channel: err=" << err;
            return;
        }
    }
}

} // namespace aos::zephyr::smclient
