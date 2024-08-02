/*
 * Copyright (C) 2024 Renesas Electronics Corporation.
 * Copyright (C) 2024 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "smclient.hpp"

namespace aos::zephyr::smclient {

/***********************************************************************************************************************
 * Public
 **********************************************************************************************************************/

aos::Error SMClient::Init(aos::sm::launcher::LauncherItf& launcher,
    aos::sm::resourcemanager::ResourceManagerItf& resourceManager, aos::monitoring::ResourceMonitorItf& resourceMonitor,
    DownloadReceiverItf& downloader, communication::ChannelManagerItf& channelManager)
{
    return aos::ErrorEnum::eNone;
}

aos::Error SMClient::InstancesRunStatus(const aos::Array<aos::InstanceStatus>& instances)
{
    return aos::ErrorEnum::eNone;
}

aos::Error SMClient::InstancesUpdateStatus(const aos::Array<aos::InstanceStatus>& instances)
{
    return aos::ErrorEnum::eNone;
}

aos::Error SMClient::SendImageContentRequest(const ImageContentRequest& request)
{
    return aos::ErrorEnum::eNone;
}

aos::Error SMClient::SendMonitoringData(const aos::monitoring::NodeMonitoringData& monitoringData)
{
    return aos::ErrorEnum::eNone;
}

aos::Error SMClient::Subscribes(aos::ConnectionSubscriberItf& subscriber)
{
    return aos::ErrorEnum::eNone;
}

void SMClient::Unsubscribes(aos::ConnectionSubscriberItf& subscriber)
{
}

aos::Error SMClient::SendClockSyncRequest()
{
    return aos::ErrorEnum::eNone;
}

void SMClient::ClockSynced()
{
}

void SMClient::ClockUnsynced()
{
}

} // namespace aos::zephyr::smclient
