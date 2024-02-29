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
#include "iamserver.hpp"

/**
 * Communication instance.
 */
class Communication : public aos::sm::launcher::InstanceStatusReceiverItf,
                      public DownloadRequesterItf,
                      public aos::monitoring::SenderItf,
                      public ClockSyncSenderItf,
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
     * @param certHandler certificate handler instance.
     * @param resourceManager resource manager instance.
     * @param resourceMonitor resource monitor instance.
     * @param downloader downloader instance.
     * @param clockSync clock sync instance.
     * @param provisioning provisioning instance.
     * @return aos::Error.
     */
    aos::Error Init(CommChannelItf& openChannel, CommChannelItf& secureChannel,
        aos::sm::launcher::LauncherItf& launcher, aos::iam::certhandler::CertHandlerItf& certHandler,
        ResourceManagerItf& resourceManager, aos::monitoring::ResourceMonitorItf& resourceMonitor,
        DownloadReceiverItf& downloader, ClockSyncItf& clockSync, ProvisioningItf& provisioning);

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
     * Sends monitoring data
     *
     * @param monitoringData monitoring data
     * @return Error
     */
    aos::Error SendMonitoringData(const aos::monitoring::NodeMonitoringData& monitoringData) override;

    /**
     * Sends clock sync request.
     *
     * @return Error.
     */
    aos::Error SendClockSyncRequest() override;

    /**
     * Notifies sender that clock is synced.
     */
    void ClockSynced() override;

    /**
     * Notifies sender that clock is unsynced.
     */
    void ClockUnsynced() override;

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
    static constexpr auto cThreadStackSize       = 16384;
    static constexpr auto cMaxSubscribers        = 2;
    static constexpr auto cConnectionTimeoutSec  = 5;
    static constexpr auto cSecureChannelCertType = "sm";

    aos::Error SendMessage(Channel channel, AosVChanSource source, const aos::String& methodName, uint64_t requestID,
        const aos::Array<uint8_t> data, aos::Error messageError) override;
    void       ConnectNotification(bool connected);
    aos::Error StartChannelThreads();
    void       ChannelHandler(Channel channel);
    aos::Error WaitTimeSynced(aos::UniqueLock& lock);
    aos::Error ConnectChannel(aos::UniqueLock& lock, Channel channel);
    aos::Error CloseChannel(Channel channel);
    size_t     GetNumConnectedChannels();
    aos::Error ProcessMessages(Channel channel);
    aos::Error ReadMessage(Channel channel, aos::Array<uint8_t>& data, size_t size);
    aos::Error WriteMessage(Channel channel, const aos::Array<uint8_t>& data);

    aos::Mutex               mMutex;
    aos::ConditionalVariable mCondVar;
    aos::Thread<aos::cDefaultFunctionMaxSize, cThreadStackSize>
        mChannelThreads[static_cast<int>(ChannelEnum::eNumChannels)];

    CommChannelItf* mChannels[static_cast<int>(ChannelEnum::eNumChannels)] {};
    bool            mClose = false;

    aos::StaticArray<aos::ConnectionSubscriberItf*, cMaxSubscribers> mConnectionSubscribers;

    ClockSyncItf*    mClockSync {};
    ProvisioningItf* mProvisioning {};

    CMClient  mCMClient;
    IAMServer mIAMServer;

    bool mClockSynced = false;
};

#endif
