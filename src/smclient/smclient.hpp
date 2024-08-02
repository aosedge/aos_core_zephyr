/*
 * Copyright (C) 2024 Renesas Electronics Corporation.
 * Copyright (C) 2024 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SMCLIENT_HPP_
#define SMCLIENT_HPP_

#include <aos/common/connectionsubsc.hpp>
#include <aos/common/monitoring.hpp>
#include <aos/sm/launcher.hpp>
#include <aos/sm/resourcemanager.hpp>

#include "clocksync/clocksync.hpp"
#include "communication/channelmanager.hpp"
#include "downloader/downloader.hpp"

namespace aos::zephyr::smclient {

/**
 * SM client instance.
 */
class SMClient : public aos::sm::launcher::InstanceStatusReceiverItf,
                 public DownloadRequesterItf,
                 public aos::monitoring::SenderItf,
                 public aos::ConnectionPublisherItf,
                 public ClockSyncSenderItf,
                 private aos::NonCopyable {
public:
    /**
     * Initializes SM client instance.
     *
     * @param launcher launcher instance.
     * @param resourceManager resource manager instance.
     * @param resourceMonitor resource monitor instance.
     * @param downloader downloader instance.
     * @return aos::Error.
     */
    aos::Error Init(aos::sm::launcher::LauncherItf&   launcher,
        aos::sm::resourcemanager::ResourceManagerItf& resourceManager,
        aos::monitoring::ResourceMonitorItf& resourceMonitor, DownloadReceiverItf& downloader,
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
    aos::Error InstancesRunStatus(const aos::Array<aos::InstanceStatus>& instances) override;

    /**
     * Sends instances update status.
     * @param instances instances status array.
     *
     * @return Error.
     */
    aos::Error InstancesUpdateStatus(const aos::Array<aos::InstanceStatus>& instances) override;

    /**
     * Send image content request
     *
     * @param request image content request
     * @return Error
     */
    aos::Error SendImageContentRequest(const ImageContentRequest& request) override;

    /**
     * Sends monitoring data
     *
     * @param monitoringData monitoring data
     * @return Error
     */
    aos::Error SendMonitoringData(const aos::monitoring::NodeMonitoringData& monitoringData) override;

    /**
     * Subscribes the provided ConnectionSubscriberItf to this object.
     *
     * @param subscriber The ConnectionSubscriberItf that wants to subscribe.
     */
    aos::Error Subscribes(aos::ConnectionSubscriberItf& subscriber) override;

    /**
     * Unsubscribes the provided ConnectionSubscriberItf from this object.
     *
     * @param subscriber The ConnectionSubscriberItf that wants to unsubscribe.
     */
    void Unsubscribes(aos::ConnectionSubscriberItf& subscriber) override;

    /**
     * Sends clock sync request.
     *
     * @return aos::Error.
     */
    aos::Error SendClockSyncRequest() override;

    /**
     * Notifies sender that clock is synced.
     */
    void ClockSynced() override;

    /**
     * Notifies sender that clock is unsynced.
     */
    void ClockUnsynced() override;
};

} // namespace aos::zephyr::smclient

#endif
