/*
 * Copyright (C) 2025 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LOGPROVIDERSTUB_HPP_
#define LOGPROVIDERSTUB_HPP_

#include <algorithm>
#include <condition_variable>
#include <mutex>
#include <vector>

#include <aos/sm/logprovider.hpp>

namespace aos::zephyr::logprovider {

/**
 * Log provider stub.
 */
class LogProviderStub : public sm::logprovider::LogProviderItf {
public:
    /**
     * Gets instance log.
     *
     * @param request request log.
     * @return Error.
     */
    Error GetInstanceLog(const cloudprotocol::RequestLog& request) override
    {
        (void)request;

        return ErrorEnum::eNone;
    }

    /**
     * Gets instance crash log.
     *
     * @param request request log.
     * @return Error.
     */
    Error GetInstanceCrashLog(const cloudprotocol::RequestLog& request) override
    {
        (void)request;

        return ErrorEnum::eNone;
    }

    /**
     * Gets system log.
     *
     * @param request request log.
     * @return Error.
     */
    Error GetSystemLog(const cloudprotocol::RequestLog& request) override
    {
        (void)request;

        return ErrorEnum::eNone;
    }

    /**
     * Subscribes on logs.
     *
     * @param observer logs observer.
     * @return Error.
     */
    Error Subscribe(sm::logprovider::LogObserverItf& observer) override
    {
        std::lock_guard<std::mutex> lock {mMutex};

        mLogObservers.push_back(&observer);

        return ErrorEnum::eNone;
    }

    /**
     * Unsubscribes from logs.
     *
     * @param observer logs observer.
     * @return Error.
     */
    Error Unsubscribe(sm::logprovider::LogObserverItf& observer) override
    {
        std::lock_guard<std::mutex> lock {mMutex};

        auto it = std::find(mLogObservers.begin(), mLogObservers.end(), &observer);
        if (it != mLogObservers.end()) {
            mLogObservers.erase(it);
            return ErrorEnum::eNone;
        }

        return ErrorEnum::eNotFound;
    }

private:
    std::mutex                                    mMutex;
    std::vector<sm::logprovider::LogObserverItf*> mLogObservers;
};

} // namespace aos::zephyr::logprovider

#endif
