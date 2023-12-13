/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <unistd.h>

#include <vchanapi.h>

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

aos::Error Communication::Init(CommChannelItf& openChannel, CommChannelItf& secureChannel)
{
    LOG_DBG() << "Initialize communication";

    mChannels[static_cast<int>(ChannelEnum::eOpen)]   = &openChannel;
    mChannels[static_cast<int>(ChannelEnum::eSecure)] = &secureChannel;

    auto err = StartChannelThreads();
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

aos::Error Communication::ProcessMessages(Channel channel)
{
    aos::StaticArray<uint8_t, sizeof(VChanMessageHeader)> readBuffer;

    auto err = mChannels[channel]->Read(readBuffer, readBuffer.MaxSize());
    if (!err.IsNone()) {
        return err;
    }

    return aos::ErrorEnum::eNone;
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
