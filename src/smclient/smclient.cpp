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

namespace {

google_protobuf_Timestamp TimestampToPB(const Time& time)
{
    auto unixTime = time.UnixTime();

    return google_protobuf_Timestamp {unixTime.tv_sec, static_cast<int32_t>(unixTime.tv_nsec)};
}

void MonitoringDataToPB(const monitoring::MonitoringData& monitoringData, const Time& timestamp,
    servicemanager_v4_MonitoringData& pbMonitoringData)
{
    pbMonitoringData.cpu              = monitoringData.mCPU + 0.5;
    pbMonitoringData.ram              = monitoringData.mRAM;
    pbMonitoringData.download         = monitoringData.mDownload;
    pbMonitoringData.upload           = monitoringData.mUpload;
    pbMonitoringData.partitions_count = monitoringData.mPartitions.Size();
    pbMonitoringData.has_timestamp    = true;
    pbMonitoringData.timestamp        = TimestampToPB(timestamp);

    for (size_t i = 0; i < monitoringData.mPartitions.Size(); i++) {
        utils::StringFromCStr(pbMonitoringData.partitions[i].name) = monitoringData.mPartitions[i].mName;
        pbMonitoringData.partitions[i].used_size                   = monitoringData.mPartitions[i].mUsedSize;
    }
}

template <typename T>
constexpr void NodeMonitoringToPB(const monitoring::NodeMonitoringData& nodeMonitoring, T& pbNodeMonitoring)
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

void InstanceStatusToPB(const InstanceStatus& aosInstanceStatus, servicemanager_v4_InstanceStatus& pbInstanceStatus)
{
    pbInstanceStatus.has_instance = true;
    utils::InstanceIdentToPB(aosInstanceStatus.mInstanceIdent, pbInstanceStatus.instance);
    utils::StringFromCStr(pbInstanceStatus.service_version) = aosInstanceStatus.mServiceVersion;
    utils::StringFromCStr(pbInstanceStatus.run_state)       = aosInstanceStatus.mRunState.ToString();

    if (!aosInstanceStatus.mError.IsNone()) {
        pbInstanceStatus.has_error = true;
        pbInstanceStatus.error     = utils::ErrorToPB(aosInstanceStatus.mError);
    }
}

class AlertVisitor : public aos::StaticVisitor<Error> {
public:
    explicit AlertVisitor(servicemanager_v4_Alert& pbAlert)
        : mPBAlert(pbAlert)
    {
    }

    Res Visit(const aos::cloudprotocol::SystemAlert& val) const
    {
        SetTimeAndTag(val, servicemanager_v4_Alert_system_alert_tag);

        auto&                                 alert = mPBAlert.AlertItem.system_alert;
        alert = servicemanager_v4_SystemAlert servicemanager_v4_SystemAlert_init_default;

        utils::StringFromCStr(alert.message) = val.mMessage;

        return ErrorEnum::eNone;
    }

    Res Visit(const aos::cloudprotocol::CoreAlert& val) const
    {
        SetTimeAndTag(val, servicemanager_v4_Alert_core_alert_tag);

        auto&                               alert = mPBAlert.AlertItem.core_alert;
        alert = servicemanager_v4_CoreAlert servicemanager_v4_CoreAlert_init_default;

        utils::StringFromCStr(alert.core_component) = val.mCoreComponent.ToString();
        utils::StringFromCStr(alert.message)        = val.mMessage;

        return ErrorEnum::eNone;
    }

    Res Visit(const aos::cloudprotocol::SystemQuotaAlert& val) const
    {
        SetTimeAndTag(val, servicemanager_v4_Alert_system_alert_tag);

        auto&                                      alert = mPBAlert.AlertItem.system_quota_alert;
        alert = servicemanager_v4_SystemQuotaAlert servicemanager_v4_SystemQuotaAlert_init_default;

        utils::StringFromCStr(alert.parameter) = val.mParameter;
        alert.value                            = val.mValue;
        utils::StringFromCStr(alert.status)    = val.mStatus.ToString();

        return ErrorEnum::eNone;
    }

    Res Visit(const aos::cloudprotocol::InstanceQuotaAlert& val) const
    {
        SetTimeAndTag(val, servicemanager_v4_Alert_instance_quota_alert_tag);

        auto&                                        alert = mPBAlert.AlertItem.instance_quota_alert;
        alert = servicemanager_v4_InstanceQuotaAlert servicemanager_v4_InstanceQuotaAlert_init_default;

        SetInstanceIndent(val, alert);

        utils::StringFromCStr(alert.parameter) = val.mParameter;
        alert.value                            = val.mValue;
        utils::StringFromCStr(alert.status)    = val.mStatus.ToString();

        return ErrorEnum::eNone;
    }

    Res Visit(const aos::cloudprotocol::DeviceAllocateAlert& val) const
    {
        SetTimeAndTag(val, servicemanager_v4_Alert_device_allocate_alert_tag);

        auto&                                         alert = mPBAlert.AlertItem.device_allocate_alert;
        alert = servicemanager_v4_DeviceAllocateAlert servicemanager_v4_DeviceAllocateAlert_init_default;

        SetInstanceIndent(val, alert);

        utils::StringFromCStr(alert.device)  = val.mDevice;
        utils::StringFromCStr(alert.message) = val.mMessage;

        return ErrorEnum::eNone;
    }

    Res Visit(const aos::cloudprotocol::ResourceValidateAlert& val) const
    {
        SetTimeAndTag(val, servicemanager_v4_Alert_resource_validate_alert_tag);

        auto&                                           alert = mPBAlert.AlertItem.resource_validate_alert;
        alert = servicemanager_v4_ResourceValidateAlert servicemanager_v4_ResourceValidateAlert_init_default;

        utils::StringFromCStr(alert.name) = val.mName;

        alert.errors_count = val.mErrors.Size();

        for (size_t i = 0; i < alert.errors_count; ++i) {
            auto& alertError = alert.errors[i];

            alertError                                = common_v1_ErrorInfo common_v1_ErrorInfo_init_default;
            alertError.aos_code                       = static_cast<int32_t>(val.mErrors[i].Value());
            alertError.exit_code                      = val.mErrors[i].Errno();
            utils::StringFromCStr(alertError.message) = val.mErrors[i].Message();
        }

        return ErrorEnum::eNone;
    }

    template <class T>
    Res Visit(const T&) const
    {
        return ErrorEnum::eNotSupported;
    }

private:
    void SetTimeAndTag(const aos::cloudprotocol::AlertItem& src,
        decltype(servicemanager_v4_Alert::which_AlertItem)  whichAlertItem) const
    {
        utils::StringFromCStr(mPBAlert.tag) = src.mTag.ToString();
        mPBAlert.has_timestamp              = true;
        mPBAlert.timestamp                  = TimestampToPB(src.mTimestamp);

        mPBAlert.which_AlertItem = whichAlertItem;
    }

    template <class T, class P>
    void SetInstanceIndent(const T& aosAlert, P& pbAlert) const
    {
        pbAlert.has_instance                               = true;
        utils::StringFromCStr(pbAlert.instance.service_id) = aosAlert.mInstanceIdent.mServiceID;
        utils::StringFromCStr(pbAlert.instance.subject_id) = aosAlert.mInstanceIdent.mSubjectID;
        pbAlert.instance.instance                          = aosAlert.mInstanceIdent.mInstance;
    }

    servicemanager_v4_Alert& mPBAlert;
};

Error AlertToPB(const cloudprotocol::AlertVariant& alert, servicemanager_v4_Alert& pbAlert)
{
    return alert.ApplyVisitor(AlertVisitor(pbAlert));
}

} // namespace

/***********************************************************************************************************************
 * Public
 **********************************************************************************************************************/

Error SMClient::Init(iam::nodeinfoprovider::NodeInfoProviderItf& nodeInfoProvider, sm::launcher::LauncherItf& launcher,
    sm::resourcemanager::ResourceManagerItf& resourceManager, monitoring::ResourceMonitorItf& resourceMonitor,
    downloader::DownloadReceiverItf& downloader, clocksync::ClockSyncItf& clockSync,
    communication::ChannelManagerItf& channelManager
#ifndef CONFIG_ZTEST
    ,
    iam::certhandler::CertHandlerItf& certHandler, crypto::CertLoaderItf& certLoader
#endif
    ,
    sm::logprovider::LogProviderItf& logProvider)
{
    LOG_DBG() << "Initialize SM client";

    mNodeInfoProvider = &nodeInfoProvider;
    mLauncher         = &launcher;
    mResourceManager  = &resourceManager;
    mResourceMonitor  = &resourceMonitor;
    mDownloader       = &downloader;
    mClockSync        = &clockSync;
    mChannelManager   = &channelManager;
#ifndef CONFIG_ZTEST
    mCertHandler = &certHandler;
    mCertLoader  = &certLoader;
#endif
    mLogProvider = &logProvider;

    auto [openChannel, err] = mChannelManager->CreateChannel(cOpenPort);
    if (!err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    if (err = mOpenHandler.Init(*openChannel, *mClockSync); !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    return ErrorEnum::eNone;
}

// cppcheck-suppress duplInheritedMember
Error SMClient::Start()
{
    LOG_DBG() << "Start SM client";

    auto nodeInfo = MakeUnique<NodeInfo>(&mAllocator);

    if (auto err = mNodeInfoProvider->GetNodeInfo(*nodeInfo); !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    mProvisioned = nodeInfo->mStatus != NodeStatusEnum::eUnprovisioned;

    if (auto err = mClockSync->Subscribe(*this); !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    if (auto err = mLogProvider->Subscribe(*this); !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    if (auto err = mNodeInfoProvider->SubscribeNodeStatusChanged(*this); !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    if (auto err = mOpenHandler.Start(); !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    if (auto err = mThread.Run([this](void*) { HandleChannel(); }); !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    return ErrorEnum::eNone;
}

// cppcheck-suppress duplInheritedMember
Error SMClient::Stop()
{
    LOG_DBG() << "Stop SM client";

    if (mOpenHandler.IsStarted()) {
        if (auto err = mOpenHandler.Stop(); !err.IsNone()) {
            LOG_ERR() << "Failed to stop open handler: err=" << err;
        }

        if (auto err = mChannelManager->DeleteChannel(cOpenPort); !err.IsNone()) {
            LOG_ERR() << "Failed to delete channel: err=" << err;
        }
    }

    if (auto err = mLogProvider->Unsubscribe(*this); !err.IsNone()) {
        LOG_ERR() << "Failed to unsubscribe log provider: err=" << err;
    }

    {
        LockGuard lock {mMutex};

        mConnectionSubscribers.Clear();

        mClose = true;
        mCondVar.NotifyOne();
    }

    return mThread.Join();
}

Error SMClient::InstancesRunStatus(const Array<InstanceStatus>& instances)
{
    LockGuard lock {mMutex};

    return SendRunStatus(instances);
}

Error SMClient::InstancesUpdateStatus(const Array<InstanceStatus>& instances)
{
    LockGuard lock {mMutex};

    LOG_INF() << "Send update instances status";

    auto  outgoingMessage         = aos::MakeUnique<servicemanager_v4_SMOutgoingMessages>(&mAllocator);
    auto& pbUpdateInstancesStatus = outgoingMessage->SMOutgoingMessage.update_instances_status;

    outgoingMessage->which_SMOutgoingMessage = servicemanager_v4_SMOutgoingMessages_update_instances_status_tag;
    pbUpdateInstancesStatus
        = servicemanager_v4_UpdateInstancesStatus servicemanager_v4_UpdateInstancesStatus_init_default;

    pbUpdateInstancesStatus.instances_count = instances.Size();

    for (size_t i = 0; i < instances.Size(); i++) {
        InstanceStatusToPB(instances[i], pbUpdateInstancesStatus.instances[i]);
    }

    return SendMessage(outgoingMessage.Get(), &servicemanager_v4_SMOutgoingMessages_msg);
}

Error SMClient::SendImageContentRequest(const downloader::ImageContentRequest& request)
{
    LockGuard lock {mMutex};

    LOG_INF() << "Send image content request: requestID=" << request.mRequestID << ", url=" << request.mURL
              << ", contentType=" << request.mContentType;

    auto  outgoingMessage       = aos::MakeUnique<servicemanager_v4_SMOutgoingMessages>(&mAllocator);
    auto& pbImageContentRequest = outgoingMessage->SMOutgoingMessage.image_content_request;

    outgoingMessage->which_SMOutgoingMessage = servicemanager_v4_SMOutgoingMessages_image_content_request_tag;
    pbImageContentRequest = servicemanager_v4_ImageContentRequest servicemanager_v4_ImageContentRequest_init_default;

    pbImageContentRequest.request_id                          = request.mRequestID;
    utils::StringFromCStr(pbImageContentRequest.content_type) = request.mContentType.ToString();
    utils::StringFromCStr(pbImageContentRequest.url)          = request.mURL;

    return SendMessage(outgoingMessage.Get(), &servicemanager_v4_SMOutgoingMessages_msg);
}

Error SMClient::SendMonitoringData(const monitoring::NodeMonitoringData& nodeMonitoring)
{
    LockGuard lock {mMutex};

    LOG_INF() << "Send node monitoring";

    auto  outgoingMessage     = MakeUnique<servicemanager_v4_SMOutgoingMessages>(&mAllocator);
    auto& pbInstantMonitoring = outgoingMessage->SMOutgoingMessage.instant_monitoring;

    outgoingMessage->which_SMOutgoingMessage = servicemanager_v4_SMOutgoingMessages_instant_monitoring_tag;
    pbInstantMonitoring = servicemanager_v4_InstantMonitoring servicemanager_v4_InstantMonitoring_init_default;

    NodeMonitoringToPB(nodeMonitoring, pbInstantMonitoring);

    return SendMessage(outgoingMessage.Get(), &servicemanager_v4_SMOutgoingMessages_msg);
}

Error SMClient::Subscribe(ConnectionSubscriberItf& subscriber)
{
    LockGuard lock(mMutex);

    if (auto err = mConnectionSubscribers.PushBack(&subscriber); !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    return aos::ErrorEnum::eNone;
}

void SMClient::Unsubscribe(ConnectionSubscriberItf& subscriber)
{
    LockGuard lock(mMutex);

    mConnectionSubscribers.Remove(&subscriber);
}

Error SMClient::OnNodeStatusChanged(const String& nodeID, const NodeStatus& status)
{
    LOG_DBG() << "Node status changed: status=" << status;

    LockGuard lock {mMutex};

    mProvisioned = status != NodeStatusEnum::eUnprovisioned;
    mCondVar.NotifyOne();

    return ErrorEnum::eNone;
}

void SMClient::OnClockSynced()
{
    LOG_DBG() << "Clock synced";

    LockGuard lock {mMutex};

    mClockSynced = true;
    mCondVar.NotifyOne();
}

void SMClient::OnClockUnsynced()
{
    LOG_DBG() << "Clock unsynced";

    LockGuard lock {mMutex};

    mClockSynced = false;
    mCondVar.NotifyOne();
}

void SMClient::OnCertChanged(const iam::certhandler::CertInfo& info)
{
    LockGuard lock {mMutex};

    LOG_DBG() << "Cert changed event received";

    mCertificateChanged = true;
    mCondVar.NotifyOne();
}

Error SMClient::SendClockSyncRequest()
{
    return mOpenHandler.SendClockSyncRequest();
}

Error SMClient::SendAlert(const cloudprotocol::AlertVariant& alert)
{
    LockGuard lock {mMutex};

    LOG_INF() << "Send alert";
    LOG_DBG() << "Send alert: alert=" << alert;

    auto outgoingMessage                     = MakeUnique<servicemanager_v4_SMOutgoingMessages>(&mAllocator);
    outgoingMessage->which_SMOutgoingMessage = servicemanager_v4_SMOutgoingMessages_alert_tag;

    auto& pbAlert = outgoingMessage->SMOutgoingMessage.alert;

    pbAlert = servicemanager_v4_Alert servicemanager_v4_Alert_init_default;

    if (auto err = AlertToPB(alert, pbAlert); !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    return SendMessage(outgoingMessage.Get(), &servicemanager_v4_SMOutgoingMessages_msg);
}

/***********************************************************************************************************************
 * Private
 **********************************************************************************************************************/

void SMClient::OnConnect()
{
    LockGuard lock {mMutex};

    LOG_INF() << "On connect notification";

    auto [version, configErr] = mResourceManager->GetNodeConfigVersion();

    if (auto err = SendNodeConfigStatus(version, configErr); !err.IsNone()) {
        LOG_ERR() << "Failed to send node config status: err=" << err;
        return;
    }

    auto lastRunStatus = MakeUnique<InstanceStatusStaticArray>(&mAllocator);

    if (auto err = mLauncher->GetCurrentRunStatus(*lastRunStatus); !err.IsNone()) {
        LOG_ERR() << "Can't get current run status: err=" << err;
        return;
    }

    if (auto err = SendRunStatus(*lastRunStatus); !err.IsNone()) {
        LOG_ERR() << "Can't send current run status: err=" << err;
        return;
    }

    for (auto& subscriber : mConnectionSubscribers) {
        subscriber->OnConnect();
    }
}

void SMClient::OnDisconnect()
{
    LockGuard lock {mMutex};

    LOG_INF() << "On disconnect notification";

    for (auto& subscriber : mConnectionSubscribers) {
        subscriber->OnDisconnect();
    }
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

    case servicemanager_v4_SMIncomingMessages_run_instances_tag:
        err = ProcessRunInstances(incomingMessages->SMIncomingMessage.run_instances);
        break;

    case servicemanager_v4_SMIncomingMessages_system_log_request_tag:
        err = ProcessSystemLogRequest(incomingMessages->SMIncomingMessage.system_log_request);
        break;

    case servicemanager_v4_SMIncomingMessages_image_content_info_tag:
        err = ProcessImageContentInfo(incomingMessages->SMIncomingMessage.image_content_info);
        break;

    case servicemanager_v4_SMIncomingMessages_image_content_tag:
        err = ProcessImageContent(incomingMessages->SMIncomingMessage.image_content);
        break;

    default:
        LOG_WRN() << "Receive unsupported message: tag=" << incomingMessages->which_SMIncomingMessage;
        break;
    }

    return err;
}

Error SMClient::OnLogReceived(const cloudprotocol::PushLog& log)
{
    LOG_DBG() << "Received log: logID=" << log.mLogID << ", part=" << log.mPart << ", status=" << log.mStatus.ToString()
              << ", size=" << log.mContent.Size();

    auto  outgoingMessage = MakeUnique<servicemanager_v4_SMOutgoingMessages>(&mAllocator);
    auto& pbLog           = outgoingMessage->SMOutgoingMessage.log;

    outgoingMessage->which_SMOutgoingMessage = servicemanager_v4_SMOutgoingMessages_log_tag;
    pbLog                                    = servicemanager_v4_LogData servicemanager_v4_LogData_init_default;

    utils::StringFromCStr(pbLog.log_id) = log.mLogID.CStr();
    utils::StringFromCStr(pbLog.status) = log.mStatus.ToString();
    pbLog.part                          = log.mPart;
    pbLog.part_count                    = log.mPartsCount;

    utils::ByteArrayToPB(
        Array<uint8_t>(reinterpret_cast<const uint8_t*>(log.mContent.begin()), log.mContent.Size()), pbLog.data);

    if (!log.mErrorInfo.IsNone()) {
        pbLog.has_error = true;
        pbLog.error     = utils::ErrorToPB(log.mErrorInfo);
    }

    return SendMessage(outgoingMessage.Get(), &servicemanager_v4_SMOutgoingMessages_msg);
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

    auto  outgoingMessage     = MakeUnique<servicemanager_v4_SMOutgoingMessages>(&mAllocator);
    auto& pbAverageMonitoring = outgoingMessage->SMOutgoingMessage.average_monitoring;

    outgoingMessage->which_SMOutgoingMessage = servicemanager_v4_SMOutgoingMessages_average_monitoring_tag;
    pbAverageMonitoring = servicemanager_v4_AverageMonitoring servicemanager_v4_AverageMonitoring_init_default;

    auto averageMonitoring = MakeUnique<monitoring::NodeMonitoringData>(&mAllocator);

    if (auto err = mResourceMonitor->GetAverageMonitoringData(*averageMonitoring); !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    NodeMonitoringToPB(*averageMonitoring, pbAverageMonitoring);

    return SendMessage(outgoingMessage.Get(), &servicemanager_v4_SMOutgoingMessages_msg);
}

Error SMClient::ProcessRunInstances(const servicemanager_v4_RunInstances& pbRunInstances)
{
    LOG_INF() << "Process run instances";

    auto aosServices = MakeUnique<ServiceInfoStaticArray>(&mAllocator);

    aosServices->Resize(pbRunInstances.services_count);

    for (auto i = 0; i < pbRunInstances.services_count; i++) {
        const auto& pbService  = pbRunInstances.services[i];
        auto&       aosService = (*aosServices)[i];

        aosService.mServiceID  = utils::StringFromCStr(pbService.service_id);
        aosService.mProviderID = utils::StringFromCStr(pbService.provider_id);
        aosService.mVersion    = utils::StringFromCStr(pbService.version);
        aosService.mGID        = pbService.gid;
        aosService.mURL        = pbService.url;
        utils::PBToByteArray(pbService.sha256, aosService.mSHA256);
        aosService.mSize = pbService.size;
    }

    auto aosLayers = MakeUnique<LayerInfoStaticArray>(&mAllocator);

    aosLayers->Resize(pbRunInstances.layers_count);

    for (auto i = 0; i < pbRunInstances.layers_count; i++) {
        const auto& pbLayer  = pbRunInstances.layers[i];
        auto&       aosLayer = (*aosLayers)[i];

        aosLayer.mLayerID     = utils::StringFromCStr(pbLayer.layer_id);
        aosLayer.mLayerDigest = utils::StringFromCStr(pbLayer.digest);
        aosLayer.mVersion     = utils::StringFromCStr(pbLayer.version);
        aosLayer.mURL         = utils::StringFromCStr(pbLayer.url);
        utils::PBToByteArray(pbLayer.sha256, aosLayer.mSHA256);
        aosLayer.mSize = pbLayer.size;
    }

    auto aosInstances = MakeUnique<InstanceInfoStaticArray>(&mAllocator);

    aosInstances->Resize(pbRunInstances.instances_count);

    for (auto i = 0; i < pbRunInstances.instances_count; i++) {
        const auto& pbInstance  = pbRunInstances.instances[i];
        auto&       aosInstance = (*aosInstances)[i];

        if (pbInstance.has_instance) {
            utils::PBToInstanceIdent(pbInstance.instance, aosInstance.mInstanceIdent);
        }

        aosInstance.mUID         = pbInstance.uid;
        aosInstance.mPriority    = pbInstance.priority;
        aosInstance.mStoragePath = utils::StringFromCStr(pbInstance.storage_path);
        aosInstance.mStatePath   = utils::StringFromCStr(pbInstance.state_path);
    }

    auto err = mLauncher->RunInstances(*aosServices, *aosLayers, *aosInstances, pbRunInstances.force_restart);
    if (!err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    return ErrorEnum::eNone;
}

Error SMClient::ProcessSystemLogRequest(const servicemanager_v4_SystemLogRequest& pbSystemLogRequest)
{
    LOG_INF() << "Process system log request";

    auto logRequest = MakeUnique<cloudprotocol::RequestLog>(&mAllocator);

    logRequest->mLogID   = utils::StringFromCStr(pbSystemLogRequest.log_id);
    logRequest->mLogType = cloudprotocol::LogTypeEnum::eSystemLog;

    if (pbSystemLogRequest.has_from) {
        logRequest->mFilter.mFrom.SetValue(Time::Unix(pbSystemLogRequest.from.seconds, pbSystemLogRequest.from.nanos));
    }
    if (pbSystemLogRequest.has_till) {
        logRequest->mFilter.mTill.SetValue(Time::Unix(pbSystemLogRequest.till.seconds, pbSystemLogRequest.till.nanos));
    }

    return mLogProvider->GetSystemLog(*logRequest);
}

Error SMClient::ProcessImageContentInfo(const servicemanager_v4_ImageContentInfo& pbImageContentInfo)
{
    LOG_INF() << "Process image content info: requestID=" << pbImageContentInfo.request_id;

    auto aosImageContentInfo = aos::MakeUnique<downloader::ImageContentInfo>(&mAllocator);

    aosImageContentInfo->mRequestID = pbImageContentInfo.request_id;
    aosImageContentInfo->mFiles.Resize(pbImageContentInfo.image_files_count);

    for (auto i = 0; i < pbImageContentInfo.image_files_count; i++) {
        aosImageContentInfo->mFiles[i].mRelativePath
            = utils::StringFromCStr(pbImageContentInfo.image_files[i].relative_path);
        utils::PBToByteArray(pbImageContentInfo.image_files[i].sha256, aosImageContentInfo->mFiles[i].mSHA256);
        aosImageContentInfo->mFiles[i].mSize = pbImageContentInfo.image_files[i].size;
    }

    if (pbImageContentInfo.has_error) {
        aosImageContentInfo->mError = utils::PBToError(pbImageContentInfo.error);
    }

    if (auto err = mDownloader->ReceiveImageContentInfo(*aosImageContentInfo); !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    return aos::ErrorEnum::eNone;
}

Error SMClient::ProcessImageContent(const servicemanager_v4_ImageContent& pbImageContent)
{
    LOG_DBG() << "Process image content: requestID=" << pbImageContent.request_id
              << ", relativePath=" << pbImageContent.relative_path;

    auto fileChunk = aos::MakeUnique<downloader::FileChunk>(&mAllocator);

    fileChunk->mRequestID    = pbImageContent.request_id;
    fileChunk->mRelativePath = utils::StringFromCStr(pbImageContent.relative_path);
    fileChunk->mPartsCount   = pbImageContent.parts_count;
    fileChunk->mPart         = pbImageContent.part;
    utils::PBToByteArray(pbImageContent.data, fileChunk->mData);

    auto err = mDownloader->ReceiveFileChunk(*fileChunk);
    if (!err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    return aos::ErrorEnum::eNone;
}

Error SMClient::SendNodeConfigStatus(const String& version, const Error& configErr)
{
    LOG_INF() << "Send node config status: version=" << version << ", configErr=" << configErr;

    auto  outgoingMessage    = MakeUnique<servicemanager_v4_SMOutgoingMessages>(&mAllocator);
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
        pbNodeConfigStatus.has_error = true;
        pbNodeConfigStatus.error     = utils::ErrorToPB(configErr);
    }

    return SendMessage(outgoingMessage.Get(), &servicemanager_v4_SMOutgoingMessages_msg);
}

Error SMClient::SendRunStatus(const Array<InstanceStatus>& instances)
{
    LOG_INF() << "Send run instances status";

    auto  outgoingMessage      = aos::MakeUnique<servicemanager_v4_SMOutgoingMessages>(&mAllocator);
    auto& pbRunInstancesStatus = outgoingMessage->SMOutgoingMessage.run_instances_status;

    outgoingMessage->which_SMOutgoingMessage = servicemanager_v4_SMOutgoingMessages_run_instances_status_tag;
    pbRunInstancesStatus = servicemanager_v4_RunInstancesStatus servicemanager_v4_RunInstancesStatus_init_default;

    pbRunInstancesStatus.instances_count = instances.Size();

    for (size_t i = 0; i < instances.Size(); i++) {
        InstanceStatusToPB(instances[i], pbRunInstancesStatus.instances[i]);
    }

    return SendMessage(outgoingMessage.Get(), &servicemanager_v4_SMOutgoingMessages_msg);
}

Error SMClient::SetupChannel()
{
    auto [channel, err] = mChannelManager->CreateChannel(cSecurePort);
    if (!err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

#ifndef CONFIG_ZTEST
    if (err = mCertHandler->SubscribeCertChanged(cSMCertType, *this); !err.IsNone()) {
        return AOS_ERROR_WRAP(Error(err, "can't subscribe on cert changed event"));
    }

    if (err = mTLSChannel.Init("sm", *mCertHandler, *mCertLoader, *channel); !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    if (err = mTLSChannel.SetTLSConfig(cSMCertType); !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    channel = &mTLSChannel;
#endif

    if (err = PBHandler::Init("SM secure", *channel); !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    if (err = communication::PBHandler<servicemanager_v4_SMIncomingMessages_size,
            servicemanager_v4_SMOutgoingMessages_size>::Start();
        !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    return ErrorEnum::eNone;
}

Error SMClient::ReleaseChannel()
{
    {
        LockGuard lock {mMutex};

        mCertificateChanged = false;
    }

    if (!IsStarted()) {
        return ErrorEnum::eNone;
    }

    if (auto err = communication::PBHandler<servicemanager_v4_SMIncomingMessages_size,
            servicemanager_v4_SMOutgoingMessages_size>::Stop();
        !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    if (auto err = mChannelManager->DeleteChannel(cSecurePort); !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

#ifndef CONFIG_ZTEST
    if (auto err = mCertHandler->UnsubscribeCertChanged(*this); !err.IsNone()) {
        return AOS_ERROR_WRAP(Error(err, "can't unsubscribe from cert changed event"));
    }
#endif

    return ErrorEnum::eNone;
}

void SMClient::HandleChannel()
{
    while (true) {
        if (auto err = ReleaseChannel(); !err.IsNone()) {
            LOG_ERR() << "Can't release channel: err=" << err;
        }

        UniqueLock lock {mMutex};

        mCondVar.Wait(lock, [this]() { return (mClockSynced && mProvisioned) || mClose; });

        if (mClose) {
            return;
        }

        if (auto err = SetupChannel(); !err.IsNone()) {
            LOG_ERR() << "Can't setup channel: err=" << err;
            LOG_DBG() << "Reconnect in " << cReconnectInterval;

            if (err = mCondVar.Wait(lock, cReconnectInterval, [this] { return mClose; });
                !err.IsNone() && !err.Is(ErrorEnum::eTimeout)) {
                LOG_ERR() << "Failed to wait reconnect: err=" << err;
            }

            continue;
        }

#if AOS_CONFIG_THREAD_STACK_USAGE
        LOG_DBG() << "Stack usage: size=" << mThread.GetStackUsage();
#endif

        mCondVar.Wait(lock, [this]() { return !mClockSynced || !mProvisioned || mClose || mCertificateChanged; });
    }
}

} // namespace aos::zephyr::smclient
