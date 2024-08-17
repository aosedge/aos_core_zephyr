/*
 * Copyright (C) 2024 Renesas Electronics Corporation.
 * Copyright (C) 2024 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "smclient.hpp"
#include "log.hpp"

namespace aos::zephyr::smclient {

/***********************************************************************************************************************
 * Public
 **********************************************************************************************************************/

Error SMClient::Init(clocksync::ClockSyncItf& clockSync, communication::ChannelManagerItf& channelManager)
{
    LOG_DBG() << "Initialize SM client";

    auto [openChannel, err] = channelManager.CreateChannel(cOpenPort);
    if (!err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    if (auto err = mOpenHandler.Init(*openChannel, clockSync); !err.IsNone()) {
        return err;
    }

    if (auto err = clockSync.Subscribe(*this); err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    return ErrorEnum::eNone;
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

Error SMClient::SendMonitoringData(const monitoring::NodeMonitoringData& monitoringData)
{
    return ErrorEnum::eNone;
}

Error SMClient::Subscribes(ConnectionSubscriberItf& subscriber)
{
    return ErrorEnum::eNone;
}

void SMClient::Unsubscribes(ConnectionSubscriberItf& subscriber)
{
}

void SMClient::OnClockSynced()
{
}

void SMClient::OnClockUnsynced()
{
}

Error SMClient::SendClockSyncRequest()
{
    return mOpenHandler.SendClockSyncRequest();
}

} // namespace aos::zephyr::smclient
