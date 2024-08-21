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
 * Public
 **********************************************************************************************************************/

Error SMClient::Init(iam::nodeinfoprovider::NodeInfoProviderItf& nodeInfoProvider,
    sm::resourcemanager::ResourceManagerItf& resourceManager, clocksync::ClockSyncItf& clockSync,
    communication::ChannelManagerItf& channelManager)
{
    LOG_DBG() << "Initialize SM client";

    mNodeInfoProvider = &nodeInfoProvider;
    mResourceManager  = &resourceManager;
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
    return ErrorEnum::eNone;
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
