/*
 * Copyright (C) 2024 Renesas Electronics Corporation.
 * Copyright (C) 2024 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SMCLIENT_HPP_
#define SMCLIENT_HPP_

#include <aos/common/connectionsubsc.hpp>
#include <aos/common/monitoring/monitoring.hpp>
#include <aos/sm/launcher.hpp>
#include <aos/sm/resourcemanager.hpp>

#include "clocksync/clocksync.hpp"
#include "communication/channelmanager.hpp"
#include "downloader/downloader.hpp"

#include "openhandler.hpp"

namespace aos::zephyr::smclient {

/**
 * SM client instance.
 */
class SMClient : public communication::PBHandler<servicemanager_v4_SMIncomingMessages_size,
                     servicemanager_v4_SMOutgoingMessages_size>,
                 public iam::nodeinfoprovider::NodeStatusObserverItf,
                 public sm::launcher::InstanceStatusReceiverItf,
                 public downloader::DownloadRequesterItf,
                 public aos::monitoring::SenderItf,
                 public ConnectionPublisherItf,
                 public clocksync::ClockSyncSenderItf,
                 public clocksync::ClockSyncSubscriberItf,
                 private NonCopyable {
public:
    /**
     * Initializes SM client instance.
     *
     * @param nodeInfoProvider node info provider instance.
     * @param launcher launcher instance.
     * @param resourceManager resource manager instance.
     * @param resourceMonitor resource monitor instance.
     * @param downloader downloader instance.
     * @param clockSync clock sync instance.
     * @param channelManager channel manager instance.
     * @return Error.
     */
    Error Init(iam::nodeinfoprovider::NodeInfoProviderItf& nodeInfoProvider, sm::launcher::LauncherItf& launcher,
        sm::resourcemanager::ResourceManagerItf& resourceManager, aos::monitoring::ResourceMonitorItf& resourceMonitor,
        downloader::DownloadReceiverItf& downloader, clocksync::ClockSyncItf& clockSync,
        communication::ChannelManagerItf& channelManager);

    /**
     * Destructor.
     */
    ~SMClient();

    /**
     * Sends instances run status.
     *
     * @param instances instances status array.
     * @return Error.
     */
    Error InstancesRunStatus(const Array<InstanceStatus>& instances) override;

    /**
     * Sends instances update status.
     *
     * @param instances instances status array.
     * @return Error.
     */
    Error InstancesUpdateStatus(const Array<InstanceStatus>& instances) override;

    /**
     * Send image content request.
     *
     * @param request image content request.
     * @return Error.
     */
    Error SendImageContentRequest(const downloader::ImageContentRequest& request) override;

    /**
     * Sends monitoring data.
     *
     * @param nodeMonitoring node monitoring data.
     * @return Error.
     */
    Error SendMonitoringData(const aos::monitoring::NodeMonitoringData& nodeMonitoring) override;

    /**
     * Subscribes the provided ConnectionSubscriberItf to this object.
     *
     * @param subscriber The ConnectionSubscriberItf that wants to subscribe.
     * @return Error.
     */
    Error Subscribes(ConnectionSubscriberItf& subscriber) override;

    /**
     * Unsubscribes the provided ConnectionSubscriberItf from this object.
     *
     * @param subscriber The ConnectionSubscriberItf that wants to unsubscribe.
     */
    void Unsubscribes(ConnectionSubscriberItf& subscriber) override;

    /**
     * Sends clock sync request.
     *
     * @return Error.
     */
    Error SendClockSyncRequest() override;

    /**
     * On node status changed event.
     *
     * @param nodeID node id
     * @param status node status
     * @return Error
     */
    Error OnNodeStatusChanged(const String& nodeID, const NodeStatus& status) override;

    /**
     * Notifies subscriber clock is synced.
     */
    void OnClockSynced() override;

    /**
     * Notifies subscriber clock is unsynced.
     */
    void OnClockUnsynced() override;

private:
    static constexpr auto cOpenPort   = CONFIG_AOS_SM_OPEN_PORT;
    static constexpr auto cSecurePort = CONFIG_AOS_SM_SECURE_PORT;

    void  OnConnect() override;
    void  OnDisconnect() override;
    Error ReceiveMessage(const Array<uint8_t>& data) override;

    void  UpdatePBHandlerState();
    Error SendNodeConfigStatus(const String& version, const Error& configErr);
    Error ProcessGetNodeConfigStatus(const servicemanager_v4_GetNodeConfigStatus& pbGetNodeConfigStatus);
    Error ProcessCheckNodeConfig(const servicemanager_v4_CheckNodeConfig& pbCheckNodeConfig);
    Error ProcessSetNodeConfig(const servicemanager_v4_SetNodeConfig& pbSetNodeConfig);
    Error ProcessGetAverageMonitoring(const servicemanager_v4_GetAverageMonitoring& pbGetAverageMonitoring);
    Error ProcessRunInstances(const servicemanager_v4_RunInstances& pbRunInstances);
    Error ProcessImageContentInfo(const servicemanager_v4_ImageContentInfo& pbImageContentInfo);
    Error ProcessImageContent(const servicemanager_v4_ImageContent& pbImageContent);

    OpenHandler                                 mOpenHandler;
    iam::nodeinfoprovider::NodeInfoProviderItf* mNodeInfoProvider {};
    sm::launcher::LauncherItf*                  mLauncher {};
    sm::resourcemanager::ResourceManagerItf*    mResourceManager {};
    aos::monitoring::ResourceMonitorItf*        mResourceMonitor {};
    downloader::DownloadReceiverItf*            mDownloader {};
    communication::ChannelManagerItf*           mChannelManager {};

    Mutex mMutex;
    bool  mClockSynced = false;
    bool  mProvisioned = false;

    StaticAllocator<sizeof(servicemanager_v4_SMIncomingMessages) + sizeof(servicemanager_v4_SMOutgoingMessages)
        + Max(sizeof(NodeInfo), sizeof(aos::monitoring::NodeMonitoringData),
            sizeof(ServiceInfoStaticArray) + sizeof(LayerInfoStaticArray) + sizeof(InstanceInfoStaticArray),
            sizeof(downloader::ImageContentInfo), sizeof(downloader::FileChunk))>
        mAllocator;
};

} // namespace aos::zephyr::smclient

#endif
