/*
 * Copyright (C) 2024 Renesas Electronics Corporation.
 * Copyright (C) 2024 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CHANNELMANAGERSTUB_HPP_
#define CHANNELMANAGERSTUB_HPP_

#include <unordered_map>

#include "communication/channelmanager.hpp"

#include "channelstub.hpp"

class ChannelManagerStub : public aos::zephyr::communication::ChannelManagerItf {
public:
    aos::RetWithError<aos::zephyr::communication::ChannelItf*> CreateChannel(uint32_t port) override
    {
        std::lock_guard lock {mMutex};

        if (mChannels.find(port) != mChannels.end()) {
            return {nullptr, aos::ErrorEnum::eAlreadyExist};
        }

        auto channel = std::make_unique<ChannelStub>();

        mChannels[port] = std::move(channel);

        mCV.notify_one();

        return mChannels[port].get();
    }

    aos::Error DeleteChannel(uint32_t port) override
    {
        std::lock_guard lock {mMutex};

        if (mChannels.find(port) == mChannels.end()) {
            return aos::ErrorEnum::eNotFound;
        }

        mChannels.erase(port);

        mCV.notify_one();

        return aos::ErrorEnum::eNone;
    }

    aos::RetWithError<ChannelStub*> GetChannel(
        uint32_t port, const std::chrono::duration<double>& timeout = std::chrono::duration<double>::zero())
    {
        std::unique_lock lock {mMutex};

        if (!mCV.wait_for(lock, timeout, [&] { return mChannels.find(port) != mChannels.end(); })) {
            return {nullptr, aos::ErrorEnum::eNotFound};
        }

        return mChannels[port].get();
    }

    aos::Error Stop()
    {
        for (auto& [port, channel] : mChannels) {
            channel->Close();
        }

        return aos::ErrorEnum::eNone;
    }

private:
    std::unordered_map<uint32_t, std::unique_ptr<ChannelStub>> mChannels;
    std::mutex                                                 mMutex;
    std::condition_variable                                    mCV;
};

#endif
