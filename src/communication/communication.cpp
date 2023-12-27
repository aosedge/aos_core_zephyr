/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <unistd.h>

#include <aos/common/tools/memory.hpp>

#include "utils/checksum.hpp"

#include "communication.hpp"
#include "log.hpp"

/***********************************************************************************************************************
 * Public
 **********************************************************************************************************************/

Communication::~Communication()
{
    {
        aos::LockGuard lock(mMutex);

        LOG_INF() << "Close communication channels";

        for (auto i = 0; i < static_cast<int>(ChannelEnum::eNumChannels); i++) {
            if (mChannels[i]->IsConnected()) {
                auto err = mChannels[i]->Close();
                if (!err.IsNone()) {
                    LOG_ERR() << "Can't close channel: " << err;
                }
            }
        }

        mClose = true;
    }

    for (auto i = 0; i < static_cast<int>(ChannelEnum::eNumChannels); i++) {
        mChannelThreads[i].Join();
    }

    {
        aos::LockGuard lock(mMutex);

        mConnectionSubscribers.Clear();
    }
}

aos::Error Communication::Init(CommChannelItf& openChannel, CommChannelItf& secureChannel,
    aos::sm::launcher::LauncherItf& launcher, ResourceManagerItf& resourceManager, DownloadReceiverItf& downloader)
{
    LOG_DBG() << "Initialize communication";

    auto err = mCMClient.Init(launcher, resourceManager, downloader, *this);
    if (!err.IsNone()) {
        return err;
    }

    mChannels[static_cast<int>(ChannelEnum::eOpen)]   = &openChannel;
    mChannels[static_cast<int>(ChannelEnum::eSecure)] = &secureChannel;

    err = StartChannelThreads();
    if (!err.IsNone()) {
        return err;
    }

    return aos::ErrorEnum::eNone;
}

aos::Error Communication::Subscribes(aos::ConnectionSubscriberItf& subscriber)
{
    aos::LockGuard lock(mMutex);

    if (mConnectionSubscribers.Size() >= cMaxSubscribers) {
        return aos::ErrorEnum::eOutOfRange;
    }

    mConnectionSubscribers.PushBack(&subscriber);

    return aos::ErrorEnum::eNone;
}

void Communication::Unsubscribes(aos::ConnectionSubscriberItf& subscriber)
{
    aos::LockGuard lock(mMutex);

    auto it = mConnectionSubscribers.Find(&subscriber);
    if (it.mError.IsNone()) {
        mConnectionSubscribers.Remove(it.mValue);
    }
}

/***********************************************************************************************************************
 * Private
 **********************************************************************************************************************/

aos::Error Communication::StartChannelThreads()
{
    auto err = mChannelThreads[static_cast<int>(ChannelEnum::eOpen)].Run(
        [this](void*) { ChannelHandler(ChannelEnum::eOpen); });
    if (!err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    err = mChannelThreads[static_cast<int>(ChannelEnum::eSecure)].Run(
        [this](void*) { ChannelHandler(ChannelEnum::eSecure); });
    if (!err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    return aos::ErrorEnum::eNone;
}

void Communication::ChannelHandler(Channel channel)
{
    while (true) {
        aos::Error err;

        {
            aos::LockGuard lock(mMutex);

            if (mClose) {
                return;
            }

            err = mChannels[channel]->Connect();
            if (err.IsNone()) {
                LOG_DBG() << "Channel connected: channel = " << channel;

                if (GetNumConnectedChannels() == static_cast<size_t>(ChannelEnum::eNumChannels)) {
                    ConnectNotification(true);
                }
            }
        }

        if (!err.IsNone()) {
            LOG_WRN() << "Can't connect to channel: " << err << ", reconnect in " << cConnectionTimeoutSec << " sec...";

            sleep(cConnectionTimeoutSec);

            continue;
        }

        err = ProcessMessages(channel);
        if (!err.IsNone()) {
            LOG_ERR() << "Error processing messages: channel = " << channel << ", err = " << err;
        }

        {
            aos::LockGuard lock(mMutex);

            if (mClose) {
                return;
            }

            err = mChannels[channel]->Close();
            if (!err.IsNone()) {
                LOG_ERR() << "Error closing channel: channel = " << channel << ", err = " << err;
            }

            LOG_DBG() << "Channel disconnected: channel = " << channel;

            if (GetNumConnectedChannels() == static_cast<size_t>(ChannelEnum::eNumChannels) - 1) {
                ConnectNotification(false);
            }
        }
    }
}

aos::Error Communication::SendMessage(Channel channel, AosVChanSource source, const aos::String& methodName,
    uint64_t requestID, const aos::Array<uint8_t> data, aos::Error messageError)
{
    LOG_DBG() << "Send message: channel = " << channel << ", source = " << source << ", methodName = " << methodName
              << ", size = " << data.Size() << " error = " << messageError;

    VChanMessageHeader header = {source, static_cast<uint32_t>(data.Size()), requestID, messageError.Errno(),
        static_cast<int32_t>(messageError.Value())};

    auto checksum = CalculateSha256(data);
    if (!checksum.mError.IsNone()) {
        return AOS_ERROR_WRAP(checksum.mError);
    }

    aos::Array<uint8_t>(reinterpret_cast<uint8_t*>(header.mSha256), aos::cSHA256Size) = checksum.mValue;

    auto err = mChannels[channel]->Write(
        aos::Array<uint8_t>(reinterpret_cast<uint8_t*>(&header), sizeof(VChanMessageHeader)));
    if (!err.IsNone()) {
        return err;
    }

    err = mChannels[channel]->Write(data);
    if (!err.IsNone()) {
        return err;
    }

    return aos::ErrorEnum::eNone;
}

aos::Error Communication::ProcessMessages(Channel channel)
{
    auto headerData = aos::StaticArray<uint8_t, sizeof(VChanMessageHeader)>();

    while (true) {
        auto err = mChannels[channel]->Read(headerData, sizeof(VChanMessageHeader));
        if (!err.IsNone()) {
            return err;
        }

        auto header = reinterpret_cast<VChanMessageHeader*>(headerData.Get());

        LOG_DBG() << "Receive message: channel = " << channel << ", source = " << header->mSource
                  << ", size = " << header->mDataSize;

        if (header->mDataSize > mCMClient.GetReceiveBufferSize()) {
            LOG_ERR() << "Not enough mem in receive buffer";
            continue;
        }

        aos::Array<uint8_t> data(static_cast<uint8_t*>(mCMClient.GetReceiveBuffer()), mCMClient.GetReceiveBufferSize());

        if (header->mDataSize) {
            err = mChannels[channel]->Read(data, header->mDataSize);
            if (!err.IsNone()) {
                return err;
            }

            auto checksum = CalculateSha256(data);
            if (!checksum.mError.IsNone()) {
                LOG_ERR() << "Can't calculate checksum: " << checksum.mError;
                continue;
            }

            if (checksum.mValue != aos::Array<uint8_t>(header->mSha256, aos::cSHA256Size)) {
                LOG_ERR() << "Checksum error";
                continue;
            }
        }

        aos::LockGuard lock(mMutex);

        switch (header->mSource) {
        case AOS_VCHAN_SM:
            err = mCMClient.ProcessMessage(header->mMethodName, header->mRequestID, data);
            break;

        case AOS_VCHAN_IAM:
            break;

        default:
            LOG_ERR() << "Wrong source received: " << header->mSource;
        }

        if (!err.IsNone()) {
            LOG_ERR() << "Error processing message: " << err;
        }
    }
}

size_t Communication::GetNumConnectedChannels()
{
    size_t count = 0;

    for (auto i = 0; i < static_cast<int>(ChannelEnum::eNumChannels); i++) {
        if (mChannels[i]->IsConnected()) {
            count++;
        }
    }

    return count;
}

void Communication::ConnectNotification(bool connected)
{
    LOG_INF() << "Connection notification: " << connected;

    for (auto& subscriber : mConnectionSubscribers) {
        if (connected) {
            subscriber->OnConnect();
        } else {
            subscriber->OnDisconnect();
        }
    }
}
