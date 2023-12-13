/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef COMMUNICATION_HPP_
#define COMMUNICATION_HPP_

#include <aos/common/connectionsubsc.hpp>
#include <aos/common/tools/array.hpp>
#include <aos/common/tools/enum.hpp>
#include <aos/common/tools/thread.hpp>

#include "channeltype.hpp"
#include "commchannel.hpp"

/**
 * Communication instance.
 */
class Communication : public aos::ConnectionPublisherItf, private aos::NonCopyable {
public:
    /**
     * Initializes communication instance.
     *
     * @param openChannel open channel instance.
     * @param secureChannel secure channel instance.
     * @return aos::Error.
     */
    aos::Error Init(CommChannelItf& openChannel, CommChannelItf& secureChannel);

    /**
     * Destructor.
     */
    ~Communication();

    /**
     * Subscribes the provided ConnectionSubscriberItf to this object.
     *
     * @param subscriber The ConnectionSubscriberItf that wants to subscribe.
     */
    virtual aos::Error Subscribes(aos::ConnectionSubscriberItf& subscriber) override;

    /**
     * Unsubscribes the provided ConnectionSubscriberItf from this object.
     *
     * @param subscriber The ConnectionSubscriberItf that wants to unsubscribe.
     */
    virtual void Unsubscribes(aos::ConnectionSubscriberItf& subscriber) override;

private:
    static constexpr auto cMaxSubscribers       = 1;
    static constexpr auto cConnectionTimeoutSec = 5;

    void       ConnectNotification(bool connected);
    aos::Error StartChannelThreads();
    void       ChannelHandler(Channel channel);
    size_t     GetNumConnectedChannels();
    aos::Error ProcessMessages(Channel channel);

    aos::Mutex    mMutex;
    aos::Thread<> mChannelThreads[static_cast<int>(ChannelEnum::eNumChannels)];

    CommChannelItf* mChannels[static_cast<int>(ChannelEnum::eNumChannels)] {};
    bool            mClose = false;

    aos::StaticArray<aos::ConnectionSubscriberItf*, cMaxSubscribers> mConnectionSubscribers;
};

#endif
