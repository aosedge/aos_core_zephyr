/*
 * Copyright (C) 2024 Renesas Electronics Corporation.
 * Copyright (C) 2024 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CHANNELMANAGERSTUB_HPP_
#define CHANNELMANAGERSTUB_HPP_

#include <unordered_map>

#include "channelstub.hpp"

class ChannelManagerStub : public aos::zephyr::communication::ChannelManagerItf {
public:
    aos::RetWithError<aos::zephyr::communication::ChannelItf*> CreateChannel(uint32_t port) override
    {
        if (mChannels.find(port) != mChannels.end()) {
            return {nullptr, aos::ErrorEnum::eAlreadyExist};
        }

        auto channel = std::make_unique<ChannelStub>();

        mChannels[port] = std::move(channel);

        return mChannels[port].get();
    }

    aos::Error DeleteChannel(uint32_t port) override
    {
        if (mChannels.find(port) == mChannels.end()) {
            return aos::ErrorEnum::eNotFound;
        }

        mChannels.erase(port);

        return aos::ErrorEnum::eNone;
    }

    aos::RetWithError<ChannelStub*> GetChannel(uint32_t port)
    {
        if (mChannels.find(port) == mChannels.end()) {
            return {nullptr, aos::ErrorEnum::eNotFound};
        }

        return mChannels[port].get();
    }

private:
    std::unordered_map<uint32_t, std::unique_ptr<ChannelStub>> mChannels;
};

#endif
