/*
 * Copyright (C) 2024 Renesas Electronics Corporation.
 * Copyright (C) 2024 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "iamclient.hpp"
#include "log.hpp"

#include "communication/pbhandler.cpp"

namespace aos::zephyr::iamclient {

/***********************************************************************************************************************
 * Public
 **********************************************************************************************************************/

Error IAMClient::Init(clocksync::ClockSyncItf& clockSync, iam::nodeinfoprovider::NodeInfoProviderItf& nodeInfoProvider,
    communication::ChannelManagerItf& channelManager)
{
    LOG_DBG() << "Initialize IAM client";

    Error err;

    if (!(err = clockSync.Subscribe(*this)).IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    if (!(err = nodeInfoProvider.SubscribeNodeStatusChanged(*this)).IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    return ErrorEnum::eNone;
}

void IAMClient::OnClockSynced()
{
}

void IAMClient::OnClockUnsynced()
{
}

Error IAMClient::OnNodeStatusChanged(const String& nodeID, const NodeStatus& status)
{
    return ErrorEnum::eNone;
}

/***********************************************************************************************************************
 * Private
 **********************************************************************************************************************/

void IAMClient::OnConnect()
{
}

void IAMClient::OnDisconnect()
{
}

Error IAMClient::ReceiveMessage(const Array<uint8_t>& data)
{
    return ErrorEnum::eNone;
}

} // namespace aos::zephyr::iamclient
