/*
 * Copyright (C) 2025 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LOGOBSERVERSTUB_HPP_
#define LOGOBSERVERSTUB_HPP_

#include <algorithm>
#include <condition_variable>
#include <mutex>
#include <queue>

#include <aos/sm/logprovider.hpp>

namespace aos::zephyr::logprovider {

/**
 * Log observer stub.
 */
class LogObserverStub : public sm::logprovider::LogObserverItf {
public:
    Error OnLogReceived(const cloudprotocol::PushLog& log) override
    {
        std::unique_lock<std::mutex> lock(mMutex);

        mLogQueue.push(log);
        mCondVar.notify_one();

        return ErrorEnum::eNone;
    }

    Error WaitLogReceived(
        cloudprotocol::PushLog& log, std::chrono::milliseconds timeout = std::chrono::milliseconds(1000))
    {
        std::unique_lock<std::mutex> lock(mMutex);

        mCondVar.wait_for(lock, timeout, [this] { return !mLogQueue.empty(); });

        if (mLogQueue.empty()) {
            return ErrorEnum::eTimeout;
        }

        log = mLogQueue.front();
        mLogQueue.pop();

        return ErrorEnum::eNone;
    }

private:
    std::mutex                         mMutex;
    std::condition_variable            mCondVar;
    std::queue<cloudprotocol::PushLog> mLogQueue;
};
} // namespace aos::zephyr::logprovider

#endif
