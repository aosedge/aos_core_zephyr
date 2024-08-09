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

#include "openhandler.hpp"

namespace aos::zephyr::smclient {

/**
 * SM client instance.
 */
class SMClient : public sm::launcher::InstanceStatusReceiverItf,
                 public DownloadRequesterItf,
                 public monitoring::SenderItf,
                 public ConnectionPublisherItf,
                 public clocksync::ClockSyncSenderItf,
                 public clocksync::ClockSyncSubscriberItf,
                 private NonCopyable {
public:
    /**
     * Initializes SM client instance.
     *
     * @param clockSync clock sync instance.
     * @param channelManager channel manager instance.
     * @return Error.
     */
    Error Init(clocksync::ClockSyncItf& clockSync, communication::ChannelManagerItf& channelManager);

    /**
     * Destructor.
     */
    ~SMClient() = default;

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
    Error SendImageContentRequest(const ImageContentRequest& request) override;

    /**
     * Sends monitoring data.
     *
     * @param monitoringData monitoring data.
     * @return Error.
     */
    Error SendMonitoringData(const monitoring::NodeMonitoringData& monitoringData) override;

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

    OpenHandler mOpenHandler;
};

} // namespace aos::zephyr::smclient

#endif
