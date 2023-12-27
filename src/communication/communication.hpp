/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef COMMUNICATION_HPP_
#define COMMUNICATION_HPP_

#include <aos/common/connectionsubsc.hpp>
#include <aos/common/tools/enum.hpp>
#include <aos/common/tools/thread.hpp>

#include "channeltype.hpp"
#include "cmclient.hpp"
#include "commchannel.hpp"
#include "messagesender.hpp"

/**
 * Communication instance.
 */
class Communication : public aos::sm::launcher::InstanceStatusReceiverItf,
                      public DownloadRequesterItf,
                      public aos::ConnectionPublisherItf,
                      private MessageSenderItf,
                      private aos::NonCopyable {
public:
    /**
     * Initializes communication instance.
     *
     * @param openChannel open channel instance.
     * @param secureChannel secure channel instance.
     * @param launcher launcher instance.
     * @param resourceManager resource manager instance.
     * @param downloader downloader instance.
     * @return aos::Error.
     */
    aos::Error Init(CommChannelItf& openChannel, CommChannelItf& secureChannel,
        aos::sm::launcher::LauncherItf& launcher, ResourceManagerItf& resourceManager, DownloadReceiverItf& downloader);

    /**
     * Destructor.
     */
    ~Communication();

    /**
     * Sends instances run status.
     *
     * @param instances instances status array.
     * @return Error.
     */
    aos::Error InstancesRunStatus(const aos::Array<aos::InstanceStatus>& instances) override;

    /**
     * Sends instances update status.
     * @param instances instances status array.
     *
     * @return Error.
     */
    aos::Error InstancesUpdateStatus(const aos::Array<aos::InstanceStatus>& instances) override;

    /**
     * Send image content request
     *
     * @param request image content request
     * @return Error
     */
    aos::Error SendImageContentRequest(const ImageContentRequest& request) override;

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

    aos::Error SendMessage(Channel channel, AosVChanSource source, const aos::String& methodName, uint64_t requestID,
        const aos::Array<uint8_t> data, aos::Error messageError) override;
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

    CMClient mCMClient;
};

#endif
