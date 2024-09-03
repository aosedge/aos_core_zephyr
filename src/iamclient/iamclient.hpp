/*
 * Copyright (C) 2024 Renesas Electronics Corporation.
 * Copyright (C) 2024 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef IAMCLIENT_HPP_
#define IAMCLIENT_HPP_

#include <aos/common/tools/thread.hpp>
#include <aos/iam/nodeinfoprovider.hpp>
#include <aos/iam/provisionmanager.hpp>

#include <proto/iamanager/v5/iamanager.pb.h>

#include "clocksync/clocksync.hpp"
#include "communication/channelmanager.hpp"
#include "communication/pbhandler.hpp"
#ifndef CONFIG_ZTEST
#include "communication/tlschannel.hpp"
#endif

namespace aos::zephyr::iamclient {

/**
 * IAM client instance.
 */
class IAMClient
    : public communication::PBHandler<iamanager_v5_IAMIncomingMessages_size, iamanager_v5_IAMOutgoingMessages_size>,
      public clocksync::ClockSyncSubscriberItf,
      public iam::nodeinfoprovider::NodeStatusObserverItf,
      private NonCopyable {
public:
    /**
     * Initializes IAM client instance.
     *
     * @param clockSync clock sync instance.
     * @param nodeInfoProvider node info provider.
     * @param provisionManager provision manager.
     * @param channelManager channel manager instance.
     * @param certHandler certificate handler.
     * @param certLoader certificate loader
     * @return Error.
     */
    Error Init(clocksync::ClockSyncItf& clockSync, iam::nodeinfoprovider::NodeInfoProviderItf& nodeInfoProvider,
        iam::provisionmanager::ProvisionManagerItf& provisionManager, communication::ChannelManagerItf& channelManager
#ifndef CONFIG_ZTEST
        ,
        iam::certhandler::CertHandlerItf& certHandler, cryptoutils::CertLoaderItf& certLoader
#endif
    );

    /**
     * Starts IAM client.
     *
     * @return Error.
     */
    Error Start();

    /**
     * Stops IAM client.
     *
     * @return Error.
     */
    Error Stop();

    /**
     * Notifies subscriber clock is synced.
     */
    void OnClockSynced() override;

    /**
     * Notifies subscriber clock is unsynced.
     */
    void OnClockUnsynced() override;

    /**
     * On node status changed event.
     *
     * @param nodeID node id
     * @param status node status
     * @return Error
     */
    Error OnNodeStatusChanged(const String& nodeID, const NodeStatus& status) override;

private:
    static constexpr auto cOpenPort          = CONFIG_AOS_IAM_OPEN_PORT;
    static constexpr auto cSecurePort        = CONFIG_AOS_IAM_SECURE_PORT;
    static constexpr auto cReconnectInterval = 5 * Time::cSeconds;
#ifndef CONFIG_ZTEST
    static constexpr auto cIAMCertType = "iam";
#endif

    void  OnConnect() override;
    void  OnDisconnect() override;
    Error ReceiveMessage(const Array<uint8_t>& data) override;

    Error ReleaseChannel();
    Error SetupChannel();
    bool  WaitChannelSwitch(UniqueLock& lock);
    bool  WaitClockSynced(UniqueLock& lock);
    bool  WaitChannelConnected(UniqueLock& lock);
    void  HandleChannels();
    template <typename T>
    Error SendError(const void* message, T& pbMessage, const Error& err);
    Error SendNodeInfo();
    Error CheckNodeIDAndStatus(const String& nodeID, const Array<NodeStatus>& expectedStatuses);
    Error ProcessStartProvisioning(const iamanager_v5_StartProvisioningRequest& pbStartProvisioningRequest);
    Error ProcessFinishProvisioning(const iamanager_v5_FinishProvisioningRequest& pbFinishProvisioningRequest);
    Error ProcessDeprovision(const iamanager_v5_DeprovisionRequest& pbDeprovisionRequest);
    Error ProcessPauseNode(const iamanager_v5_PauseNodeRequest& pbPauseNodeRequest);
    Error ProcessResumeNode(const iamanager_v5_ResumeNodeRequest& pbResumeNodeRequest);
    Error ProcessGetCertTypes(const iamanager_v5_GetCertTypesRequest& pbGetCertTypesRequest);
    Error ProcessCreateKey(const iamanager_v5_CreateKeyRequest& pbCreateKeyRequest);
    Error ProcessApplyCert(const iamanager_v5_ApplyCertRequest& pbApplyCertRequest);

    clocksync::ClockSyncItf*                    mClockSync {};
    iam::nodeinfoprovider::NodeInfoProviderItf* mNodeInfoProvider {};
    iam::provisionmanager::ProvisionManagerItf* mProvisionManager {};
    communication::ChannelManagerItf*           mChannelManager {};
#ifndef CONFIG_ZTEST
    iam::certhandler::CertHandlerItf* mCertHandler {};
    cryptoutils::CertLoaderItf*       mCertLoader {};
    communication::TLSChannel         mTLSChannel;
#endif

    NodeInfo            mNodeInfo;
    Mutex               mMutex;
    Thread<>            mThread;
    ConditionalVariable mCondVar;
    bool                mClockSynced   = false;
    bool                mSwitchChannel = false;
    int                 mCurrentPort   = 0;
    bool                mClose         = false;

    StaticAllocator<sizeof(iamanager_v5_IAMIncomingMessages) + sizeof(iamanager_v5_IAMOutgoingMessages) * 2> mAllocator;
};

} // namespace aos::zephyr::iamclient

#endif
