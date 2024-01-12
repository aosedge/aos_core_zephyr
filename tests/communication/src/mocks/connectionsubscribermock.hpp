/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CONNECTIONSUBSCRIBERMOCK_HPP_
#define CONNECTIONSUBSCRIBERMOCK_HPP_

#include <condition_variable>
#include <mutex>

#include <aos/common/connectionsubsc.hpp>

class ConnectionSubscriberMock : public aos::ConnectionSubscriberItf {
public:
    void OnConnect() override
    {
        std::lock_guard<std::mutex> lock(mMutex);

        mConnected = true;
        mConnect   = true;
        mCV.notify_one();
    }

    void OnDisconnect() override
    {
        std::lock_guard<std::mutex> lock(mMutex);

        mConnected  = false;
        mDisconnect = true;
        mCV.notify_one();
    }

    aos::Error WaitConnect(const std::chrono::duration<double>& timeout)
    {
        std::unique_lock<std::mutex> lock(mMutex);

        if (!mCV.wait_for(lock, timeout, [&] { return mConnect; })) {
            return aos::ErrorEnum::eTimeout;
        }

        mConnect = false;

        return aos::ErrorEnum::eNone;
    }

    aos::Error WaitDisconnect(const std::chrono::duration<double>& timeout)
    {
        std::unique_lock<std::mutex> lock(mMutex);

        if (!mCV.wait_for(lock, timeout, [&] { return mDisconnect; })) {
            return aos::ErrorEnum::eTimeout;
        }

        mDisconnect = false;

        return aos::ErrorEnum::eNone;
    }

    bool IsConnected() const
    {
        std::lock_guard<std::mutex> lock(mMutex);
        return mConnected;
    }

    void Clear()
    {
        std::lock_guard<std::mutex> lock(mMutex);

        mConnected  = false;
        mConnect    = false;
        mDisconnect = false;
    }

private:
    bool                    mConnected  = false;
    bool                    mConnect    = false;
    bool                    mDisconnect = false;
    std::condition_variable mCV;
    mutable std::mutex      mMutex;
};

#endif
