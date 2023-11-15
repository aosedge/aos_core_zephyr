/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "communication.hpp"
#include "log.hpp"

/***********************************************************************************************************************
 * Public
 **********************************************************************************************************************/

Communication::~Communication()
{
    aos::LockGuard lock(mMutex);

    mConnectionSubscribers.Clear();
}

aos::Error Communication::Init()
{
    LOG_DBG() << "Initialize communication";

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
