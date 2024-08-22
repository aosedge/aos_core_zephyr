/*
 * Copyright (C) 2024 Renesas Electronics Corporation.
 * Copyright (C) 2024 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <pb_decode.h>

#include "utils/pbconvert.hpp"

#include "iamclient.hpp"
#include "log.hpp"

#include "communication/pbhandler.cpp"

namespace aos::zephyr::iamclient {

/***********************************************************************************************************************
 * Public
 **********************************************************************************************************************/

Error IAMClient::Init(clocksync::ClockSyncItf& clockSync, iam::nodeinfoprovider::NodeInfoProviderItf& nodeInfoProvider,
    iam::provisionmanager::ProvisionManagerItf& provisionManager, communication::ChannelManagerItf& channelManager
#ifndef CONFIG_ZTEST
    ,
    iam::certhandler::CertHandlerItf& certHandler, cryptoutils::CertLoaderItf& certLoader
#endif
)
{
    LOG_DBG() << "Initialize IAM client";

    mClockSync        = &clockSync;
    mNodeInfoProvider = &nodeInfoProvider;
    mProvisionManager = &provisionManager;
    mChannelManager   = &channelManager;
#ifndef CONFIG_ZTEST
    mCertHandler = &certHandler;
    mCertLoader  = &certLoader;
#endif

    if (auto err = clockSync.Subscribe(*this); !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    if (auto err = nodeInfoProvider.SubscribeNodeStatusChanged(*this); !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    if (auto err = mNodeInfoProvider->GetNodeInfo(mNodeInfo); !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    auto err = mThread.Run([this](void*) { HandleChannels(); });
    if (!err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    if (mNodeInfo.mStatus == NodeStatusEnum::eUnprovisioned) {
        LOG_INF() << "Node is unprovisioned";
    }

    return ErrorEnum::eNone;
}

void IAMClient::OnClockSynced()
{
    LockGuard lock {mMutex};

    LOG_DBG() << "Clock synced";

    mClockSynced = true;
    mCondVar.NotifyOne();
}

void IAMClient::OnClockUnsynced()
{
    LockGuard lock {mMutex};

    LOG_WRN() << "Clock unsynced";

    mClockSynced = false;
    mCondVar.NotifyOne();
}

Error IAMClient::OnNodeStatusChanged(const String& nodeID, const NodeStatus& status)
{
    LOG_DBG() << "Node status changed: nodeID=" << nodeID << ", status=" << status;

    if (nodeID != mNodeInfo.mNodeID) {
        LOG_ERR() << "Wrong node ID: nodeID=" << nodeID;
    }

    if (mNodeInfo.mStatus == status) {
        return ErrorEnum::eNone;
    }

    auto sendNodeInfo = true;

    if (mNodeInfo.mStatus == NodeStatusEnum::eUnprovisioned || status == NodeStatusEnum::eUnprovisioned) {
        sendNodeInfo   = false;
        mSwitchChannel = true;
        mCondVar.NotifyOne();
    }

    mNodeInfo.mStatus = status;

    if (sendNodeInfo) {
        if (auto err = SendNodeInfo(); !err.IsNone()) {
            return err;
        }
    }

    return ErrorEnum::eNone;
}

IAMClient::~IAMClient()
{
    if (auto err = Stop(); !err.IsNone()) {
        LOG_ERR() << "Can't stop IAM handler: err=" << err;
    }

    mClockSync->Unsubscribe(*this);
    mNodeInfoProvider->UnsubscribeNodeStatusChanged(*this);

    {
        LockGuard lock {mMutex};

        mClose = true;
        mCondVar.NotifyOne();
    }

    mThread.Join();
}

/***********************************************************************************************************************
 * Private
 **********************************************************************************************************************/

void IAMClient::OnConnect()
{
    LockGuard lock {mMutex};

    LOG_DBG() << "Channel connected: port=" << mCurrentPort;

    mConnected = true;
    mCondVar.NotifyOne();
}

void IAMClient::OnDisconnect()
{
    LockGuard lock {mMutex};

    LOG_DBG() << "Channel disconnected: port=" << mCurrentPort;

    mConnected = false;
    mCondVar.NotifyOne();
}

Error IAMClient::ReleaseChannel()
{
    if (!mCurrentPort) {
        return ErrorEnum::eNone;
    }

    Error err;

    LOG_DBG() << "Release channel: port=" << mCurrentPort;

    if (auto stopErr = Stop(); !stopErr.IsNone() && err.IsNone()) {
        err = AOS_ERROR_WRAP(stopErr);
    }

    if (auto deleteErr = mChannelManager->DeleteChannel(mCurrentPort); !deleteErr.IsNone() && err.IsNone()) {
        err = AOS_ERROR_WRAP(deleteErr);
    }

    return err;
}

Error IAMClient::SetupChannel()
{
    communication::ChannelItf* channel {};
    Error                      err;

    if (mNodeInfo.mStatus == NodeStatusEnum::eUnprovisioned) {
        LOG_DBG() << "Setup open channel: port=" << cOpenPort;

        if (Tie(channel, err) = mChannelManager->CreateChannel(cOpenPort); !err.IsNone()) {
            return AOS_ERROR_WRAP(err);
        }

        mCurrentPort = cOpenPort;

        if (err = PBHandler::Init("IAM open", *channel); !err.IsNone()) {
            return AOS_ERROR_WRAP(err);
        }
    } else {
        LOG_DBG() << "Setup secure channel: port=" << cSecurePort;

        if (Tie(channel, err) = mChannelManager->CreateChannel(cSecurePort); !err.IsNone()) {
            return AOS_ERROR_WRAP(err);
        }

        mCurrentPort = cSecurePort;

#ifndef CONFIG_ZTEST
        if (err = mTLSChannel.Init(*mCertHandler, *mCertLoader, *channel); !err.IsNone()) {
            return AOS_ERROR_WRAP(err);
        }

        if (err = mTLSChannel.SetTLSConfig(cIAMCertType); !err.IsNone()) {
            return AOS_ERROR_WRAP(err);
        }
#endif

        if (err = PBHandler::Init("IAM secure", *channel); !err.IsNone()) {
            return AOS_ERROR_WRAP(err);
        }
    }

    if (err = Start(); !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    return ErrorEnum::eNone;
}

bool IAMClient::WaitChannelSwitch(UniqueLock& lock)
{
    mCondVar.Wait(lock, [this]() { return mSwitchChannel || mClose; });
    mSwitchChannel = false;

    return !mClose;
}

bool IAMClient::WaitClockSynced(UniqueLock& lock)
{
    if (mNodeInfo.mStatus != NodeStatusEnum::eUnprovisioned) {
        mCondVar.Wait(lock, [this]() { return mClockSynced || mClose; });
        mClockSynced = false;
    }

    return !mClose;
}

bool IAMClient::WaitChannelConnected(UniqueLock& lock)
{
    mCondVar.Wait(lock, [this]() { return mConnected || mClose; });

    return !mClose;
}

void IAMClient::HandleChannels()
{
    while (true) {
        if (auto err = ReleaseChannel(); !err.IsNone()) {
            LOG_ERR() << "Can't release channel: err=" << err;
        }

        UniqueLock lock {mMutex};

        if (mClose) {
            return;
        }

        if (!WaitClockSynced(lock)) {
            continue;
        }

        if (auto err = SetupChannel(); !err.IsNone()) {
            LOG_ERR() << "Can't setup channel: err=" << err;

            usleep(cReconnectInterval / 1000);

            continue;
        }

        if (!WaitChannelConnected(lock)) {
            continue;
        }

        if (auto err = SendNodeInfo(); !err.IsNone()) {
            LOG_ERR() << "Can't send node info: err=" << err;

            mSwitchChannel = true;
        }

        if (!WaitChannelSwitch(lock)) {
            continue;
        }
    }
}

Error IAMClient::ReceiveMessage(const Array<uint8_t>& data)
{
    LockGuard lock {mMutex};

    auto incomingMessages = MakeUnique<iamanager_v5_IAMIncomingMessages>(&mAllocator);
    auto stream           = pb_istream_from_buffer(data.Get(), data.Size());

    if (auto status = pb_decode(&stream, &iamanager_v5_IAMIncomingMessages_msg, incomingMessages.Get()); !status) {
        return AOS_ERROR_WRAP(Error(ErrorEnum::eRuntime, "failed to decode message"));
    }

    Error err;

    switch (incomingMessages->which_IAMIncomingMessage) {
    case iamanager_v5_IAMIncomingMessages_start_provisioning_request_tag:
        err = ProcessStartProvisioning(incomingMessages->IAMIncomingMessage.start_provisioning_request);
        break;

    case iamanager_v5_IAMIncomingMessages_finish_provisioning_request_tag:
        err = ProcessFinishProvisioning(incomingMessages->IAMIncomingMessage.finish_provisioning_request);
        break;

    case iamanager_v5_IAMIncomingMessages_deprovision_request_tag:
        err = ProcessDeprovision(incomingMessages->IAMIncomingMessage.deprovision_request);
        break;

    case iamanager_v5_IAMIncomingMessages_get_cert_types_request_tag:
        err = ProcessGetCertTypes(incomingMessages->IAMIncomingMessage.get_cert_types_request);
        break;

    case iamanager_v5_IAMIncomingMessages_create_key_request_tag:
        err = ProcessCreateKey(incomingMessages->IAMIncomingMessage.create_key_request);
        break;

    case iamanager_v5_IAMIncomingMessages_apply_cert_request_tag:
        err = ProcessApplyCert(incomingMessages->IAMIncomingMessage.apply_cert_request);
        break;

    case iamanager_v5_IAMIncomingMessages_pause_node_request_tag:
        err = ProcessPauseNode(incomingMessages->IAMIncomingMessage.pause_node_request);
        break;

    case iamanager_v5_IAMIncomingMessages_resume_node_request_tag:
        err = ProcessResumeNode(incomingMessages->IAMIncomingMessage.resume_node_request);
        break;

    default:
        LOG_WRN() << "Receive unsupported message: tag=" << incomingMessages->which_IAMIncomingMessage;
        break;
    }

    return err;
}

template <typename T>
Error IAMClient::SendError(const void* message, T& pbMessage, const Error& err)
{
    LOG_ERR() << "Process message error: err=" << err;

    pbMessage.has_error = true;
    pbMessage.error     = utils::ErrorToPB(err);

    return SendMessage(message, &iamanager_v5_IAMOutgoingMessages_msg);
}

Error IAMClient::CheckNodeIDAndStatus(const String& nodeID, const Array<NodeStatus>& expectedStatuses)
{
    if (nodeID != mNodeInfo.mNodeID) {
        return Error(ErrorEnum::eInvalidArgument, "wrong node ID");
    }

    if (auto [_, err] = expectedStatuses.Find(mNodeInfo.mStatus); !err.IsNone()) {
        return Error(ErrorEnum::eWrongState, "wrong node status");
    }

    return ErrorEnum::eNone;
}

Error IAMClient::SendNodeInfo()
{
    LOG_DBG() << "Send node info: status=" << mNodeInfo.mStatus;

    auto  outgoingMessage = aos::MakeUnique<iamanager_v5_IAMOutgoingMessages>(&mAllocator);
    auto& pbNodeInfo      = outgoingMessage->IAMOutgoingMessage.node_info;

    outgoingMessage->which_IAMOutgoingMessage = iamanager_v5_IAMOutgoingMessages_node_info_tag;
    pbNodeInfo                                = iamanager_v5_NodeInfo iamanager_v5_NodeInfo_init_default;

    utils::StringFromCStr(pbNodeInfo.node_id)   = mNodeInfo.mNodeID;
    utils::StringFromCStr(pbNodeInfo.node_type) = mNodeInfo.mNodeType;
    utils::StringFromCStr(pbNodeInfo.name)      = mNodeInfo.mName;
    utils::StringFromCStr(pbNodeInfo.status)    = mNodeInfo.mStatus.ToString();
    utils::StringFromCStr(pbNodeInfo.os_type)   = mNodeInfo.mOSType;
    pbNodeInfo.max_dmips                        = mNodeInfo.mMaxDMIPS;
    pbNodeInfo.total_ram                        = mNodeInfo.mTotalRAM;

    pbNodeInfo.cpus_count = mNodeInfo.mCPUs.Size();

    for (size_t i = 0; i < mNodeInfo.mCPUs.Size(); i++) {
        utils::StringFromCStr(pbNodeInfo.cpus[i].model_name)  = mNodeInfo.mCPUs[i].mModelName;
        pbNodeInfo.cpus[i].num_cores                          = mNodeInfo.mCPUs[i].mNumCores;
        pbNodeInfo.cpus[i].num_threads                        = mNodeInfo.mCPUs[i].mNumThreads;
        utils::StringFromCStr(pbNodeInfo.cpus[i].arch)        = mNodeInfo.mCPUs[i].mArch;
        utils::StringFromCStr(pbNodeInfo.cpus[i].arch_family) = mNodeInfo.mCPUs[i].mArchFamily;
        pbNodeInfo.cpus[i].max_dmips                          = mNodeInfo.mCPUs[i].mMaxDMIPS;
    }

    pbNodeInfo.partitions_count = mNodeInfo.mPartitions.Size();

    for (size_t i = 0; i < mNodeInfo.mPartitions.Size(); i++) {
        utils::StringFromCStr(pbNodeInfo.partitions[i].name) = mNodeInfo.mPartitions[i].mName;
        pbNodeInfo.partitions[i].total_size                  = mNodeInfo.mPartitions[i].mTotalSize;
        utils::StringFromCStr(pbNodeInfo.partitions[i].path) = mNodeInfo.mPartitions[i].mPath;

        pbNodeInfo.partitions[i].types_count = mNodeInfo.mPartitions[i].mTypes.Size();

        for (size_t j = 0; j < mNodeInfo.mPartitions[i].mTypes.Size(); j++) {
            utils::StringFromCStr(pbNodeInfo.partitions[i].types[j]) = mNodeInfo.mPartitions[i].mTypes[j];
        }
    }

    pbNodeInfo.attrs_count = mNodeInfo.mAttrs.Size();

    for (size_t i = 0; i < mNodeInfo.mAttrs.Size(); i++) {
        utils::StringFromCStr(pbNodeInfo.attrs[i].name)  = mNodeInfo.mAttrs[i].mName;
        utils::StringFromCStr(pbNodeInfo.attrs[i].value) = mNodeInfo.mAttrs[i].mValue;
    }

    return SendMessage(outgoingMessage.Get(), &iamanager_v5_IAMOutgoingMessages_msg);
}

Error IAMClient::ProcessStartProvisioning(const iamanager_v5_StartProvisioningRequest& pbStartProvisioningRequest)
{
    LOG_INF() << "Process start provisioning request";

    auto  outgoingMessage          = aos::MakeUnique<iamanager_v5_IAMOutgoingMessages>(&mAllocator);
    auto& pbStartProvisionResponse = outgoingMessage->IAMOutgoingMessage.start_provisioning_response;

    outgoingMessage->which_IAMOutgoingMessage = iamanager_v5_IAMOutgoingMessages_start_provisioning_response_tag;
    pbStartProvisionResponse
        = iamanager_v5_StartProvisioningResponse iamanager_v5_StartProvisioningResponse_init_default;

    NodeStatus expectedStatuses[] {NodeStatusEnum::eUnprovisioned};

    if (auto err = CheckNodeIDAndStatus(pbStartProvisioningRequest.node_id, utils::ToArray(expectedStatuses));
        !err.IsNone()) {
        return SendError(outgoingMessage.Get(), pbStartProvisionResponse, AOS_ERROR_WRAP(err));
    }

    if (auto err = mProvisionManager->StartProvisioning(pbStartProvisioningRequest.password); !err.IsNone()) {
        return SendError(outgoingMessage.Get(), pbStartProvisionResponse, AOS_ERROR_WRAP(err));
    }

    return SendMessage(outgoingMessage.Get(), &iamanager_v5_IAMOutgoingMessages_msg);
}

Error IAMClient::ProcessFinishProvisioning(const iamanager_v5_FinishProvisioningRequest& pbFinishProvisioningRequest)
{
    LOG_INF() << "Process finish provisioning request";

    auto  outgoingMessage           = aos::MakeUnique<iamanager_v5_IAMOutgoingMessages>(&mAllocator);
    auto& pbFinishProvisionResponse = outgoingMessage->IAMOutgoingMessage.finish_provisioning_response;

    outgoingMessage->which_IAMOutgoingMessage = iamanager_v5_IAMOutgoingMessages_finish_provisioning_response_tag;
    pbFinishProvisionResponse
        = iamanager_v5_FinishProvisioningResponse iamanager_v5_FinishProvisioningResponse_init_default;

    NodeStatus expectedStatuses[] {NodeStatusEnum::eUnprovisioned};

    if (auto err = CheckNodeIDAndStatus(pbFinishProvisioningRequest.node_id, utils::ToArray(expectedStatuses));
        !err.IsNone()) {
        return SendError(outgoingMessage.Get(), pbFinishProvisionResponse, AOS_ERROR_WRAP(err));
    }

    if (auto err = mProvisionManager->FinishProvisioning(pbFinishProvisioningRequest.password); !err.IsNone()) {
        return SendError(outgoingMessage.Get(), pbFinishProvisionResponse, AOS_ERROR_WRAP(err));
    }

    if (auto err = mNodeInfoProvider->SetNodeStatus(NodeStatusEnum::eProvisioned); !err.IsNone()) {
        return SendError(outgoingMessage.Get(), pbFinishProvisionResponse, AOS_ERROR_WRAP(err));
    }

    return SendMessage(outgoingMessage.Get(), &iamanager_v5_IAMOutgoingMessages_msg);
}

Error IAMClient::ProcessDeprovision(const iamanager_v5_DeprovisionRequest& pbDeprovisionRequest)
{
    LOG_INF() << "Process deprovision request";

    auto  outgoingMessage       = aos::MakeUnique<iamanager_v5_IAMOutgoingMessages>(&mAllocator);
    auto& pbDeprovisionResponse = outgoingMessage->IAMOutgoingMessage.deprovision_response;

    outgoingMessage->which_IAMOutgoingMessage = iamanager_v5_IAMOutgoingMessages_deprovision_response_tag;
    pbDeprovisionResponse = iamanager_v5_DeprovisionResponse iamanager_v5_DeprovisionResponse_init_default;

    NodeStatus expectedStatuses[] {NodeStatusEnum::eProvisioned, NodeStatusEnum::ePaused};

    if (auto err = CheckNodeIDAndStatus(pbDeprovisionRequest.node_id, utils::ToArray(expectedStatuses));
        !err.IsNone()) {
        return SendError(outgoingMessage.Get(), pbDeprovisionResponse, AOS_ERROR_WRAP(err));
    }

    if (auto err = mProvisionManager->Deprovision(pbDeprovisionRequest.password); !err.IsNone()) {
        return SendError(outgoingMessage.Get(), pbDeprovisionResponse, AOS_ERROR_WRAP(err));
    }

    if (auto err = mNodeInfoProvider->SetNodeStatus(NodeStatusEnum::eUnprovisioned); !err.IsNone()) {
        return SendError(outgoingMessage.Get(), pbDeprovisionResponse, AOS_ERROR_WRAP(err));
    }

    return SendMessage(outgoingMessage.Get(), &iamanager_v5_IAMOutgoingMessages_msg);
}

Error IAMClient::ProcessGetCertTypes(const iamanager_v5_GetCertTypesRequest& pbGetCertTypesRequest)
{
    LOG_INF() << "Process get cert types";

    auto  outgoingMessage        = aos::MakeUnique<iamanager_v5_IAMOutgoingMessages>(&mAllocator);
    auto& pbGetCertTypesResponse = outgoingMessage->IAMOutgoingMessage.cert_types_response;

    outgoingMessage->which_IAMOutgoingMessage = iamanager_v5_IAMOutgoingMessages_cert_types_response_tag;
    pbGetCertTypesResponse                    = iamanager_v5_CertTypes iamanager_v5_CertTypes_init_default;

    NodeStatus expectedStatuses[] {
        NodeStatusEnum::eUnprovisioned, NodeStatusEnum::eProvisioned, NodeStatusEnum::ePaused};

    if (auto err = CheckNodeIDAndStatus(pbGetCertTypesRequest.node_id, utils::ToArray(expectedStatuses));
        !err.IsNone()) {
        LOG_ERR() << "Wrong get cert types condition: err=" << err;
    }

    auto certTypes = mProvisionManager->GetCertTypes();
    if (!certTypes.mError.IsNone()) {
        LOG_ERR() << "Getting cert types error: err=" << certTypes.mError;
    }

    pbGetCertTypesResponse.types_count = certTypes.mValue.Size();

    for (size_t i = 0; i < certTypes.mValue.Size(); i++) {
        utils::StringFromCStr(pbGetCertTypesResponse.types[i]) = certTypes.mValue[i];
    }

    return SendMessage(outgoingMessage.Get(), &iamanager_v5_IAMOutgoingMessages_msg);
}

Error IAMClient::ProcessCreateKey(const iamanager_v5_CreateKeyRequest& pbCreateKeyRequest)
{
    LOG_INF() << "Process create key: type=" << pbCreateKeyRequest.type << ", subject=" << pbCreateKeyRequest.subject;

    auto  outgoingMessage     = aos::MakeUnique<iamanager_v5_IAMOutgoingMessages>(&mAllocator);
    auto& pbCreateKeyResponse = outgoingMessage->IAMOutgoingMessage.create_key_response;

    outgoingMessage->which_IAMOutgoingMessage = iamanager_v5_IAMOutgoingMessages_create_key_response_tag;
    pbCreateKeyResponse = iamanager_v5_CreateKeyResponse iamanager_v5_CreateKeyResponse_init_default;

    NodeStatus expectedStatuses[] {
        NodeStatusEnum::eUnprovisioned, NodeStatusEnum::eProvisioned, NodeStatusEnum::ePaused};

    if (auto err = CheckNodeIDAndStatus(pbCreateKeyRequest.node_id, utils::ToArray(expectedStatuses)); !err.IsNone()) {
        return SendError(outgoingMessage.Get(), pbCreateKeyResponse, AOS_ERROR_WRAP(err));
    }

    auto csr = utils::StringFromCStr(pbCreateKeyResponse.csr);

    if (auto err = mProvisionManager->CreateKey(
            pbCreateKeyRequest.type, pbCreateKeyRequest.subject, pbCreateKeyRequest.password, csr);
        !err.IsNone()) {
        return SendError(outgoingMessage.Get(), pbCreateKeyResponse, AOS_ERROR_WRAP(err));
    }

    utils::StringFromCStr(pbCreateKeyResponse.type)    = pbCreateKeyRequest.type;
    utils::StringFromCStr(pbCreateKeyResponse.node_id) = mNodeInfo.mNodeID;

    return SendMessage(outgoingMessage.Get(), &iamanager_v5_IAMOutgoingMessages_msg);
}

Error IAMClient::ProcessApplyCert(const iamanager_v5_ApplyCertRequest& pbApplyCertRequest)
{
    LOG_INF() << "Process apply cert: type=" << pbApplyCertRequest.type;

    auto  outgoingMessage     = aos::MakeUnique<iamanager_v5_IAMOutgoingMessages>(&mAllocator);
    auto& pbApplyCertResponse = outgoingMessage->IAMOutgoingMessage.apply_cert_response;

    outgoingMessage->which_IAMOutgoingMessage = iamanager_v5_IAMOutgoingMessages_apply_cert_response_tag;
    pbApplyCertResponse = iamanager_v5_ApplyCertResponse iamanager_v5_ApplyCertResponse_init_default;

    NodeStatus expectedStatuses[] {
        NodeStatusEnum::eUnprovisioned, NodeStatusEnum::eProvisioned, NodeStatusEnum::ePaused};

    if (auto err = CheckNodeIDAndStatus(pbApplyCertRequest.node_id, utils::ToArray(expectedStatuses)); !err.IsNone()) {
        return SendError(outgoingMessage.Get(), pbApplyCertResponse, AOS_ERROR_WRAP(err));
    }

    iam::certhandler::CertInfo certInfo {};

    if (auto err = mProvisionManager->ApplyCert(pbApplyCertRequest.type, pbApplyCertRequest.cert, certInfo);
        !err.IsNone()) {
        return SendError(outgoingMessage.Get(), pbApplyCertResponse, AOS_ERROR_WRAP(err));
    }

    utils::StringFromCStr(pbApplyCertResponse.type)     = pbApplyCertRequest.type;
    utils::StringFromCStr(pbApplyCertResponse.cert_url) = certInfo.mCertURL;
    utils::StringFromCStr(pbApplyCertResponse.node_id)  = mNodeInfo.mNodeID;

    if (auto err = utils::StringFromCStr(pbApplyCertResponse.serial).ByteArrayToHex(certInfo.mSerial); !err.IsNone()) {
        return SendError(outgoingMessage.Get(), pbApplyCertResponse, AOS_ERROR_WRAP(err));
    }

    return SendMessage(outgoingMessage.Get(), &iamanager_v5_IAMOutgoingMessages_msg);
}

Error IAMClient::ProcessPauseNode(const iamanager_v5_PauseNodeRequest& pbPauseNodeRequest)
{
    LOG_INF() << "Process pause node request";

    auto  outgoingMessage     = aos::MakeUnique<iamanager_v5_IAMOutgoingMessages>(&mAllocator);
    auto& pbPauseNodeResponse = outgoingMessage->IAMOutgoingMessage.pause_node_response;

    outgoingMessage->which_IAMOutgoingMessage = iamanager_v5_IAMOutgoingMessages_pause_node_response_tag;
    pbPauseNodeResponse = iamanager_v5_PauseNodeResponse iamanager_v5_PauseNodeResponse_init_default;

    NodeStatus expectedStatuses[] {NodeStatusEnum::eProvisioned};

    if (auto err = CheckNodeIDAndStatus(pbPauseNodeRequest.node_id, utils::ToArray(expectedStatuses)); !err.IsNone()) {
        return SendError(outgoingMessage.Get(), pbPauseNodeResponse, AOS_ERROR_WRAP(err));
    }

    if (auto err = mNodeInfoProvider->SetNodeStatus(NodeStatusEnum::ePaused); !err.IsNone()) {
        return SendError(outgoingMessage.Get(), pbPauseNodeResponse, AOS_ERROR_WRAP(err));
    }

    return SendMessage(outgoingMessage.Get(), &iamanager_v5_IAMOutgoingMessages_msg);
}

Error IAMClient::ProcessResumeNode(const iamanager_v5_ResumeNodeRequest& pbResumeNodeRequest)
{
    LOG_INF() << "Process resume node request";

    auto  outgoingMessage      = aos::MakeUnique<iamanager_v5_IAMOutgoingMessages>(&mAllocator);
    auto& pbResumeNodeResponse = outgoingMessage->IAMOutgoingMessage.resume_node_response;

    outgoingMessage->which_IAMOutgoingMessage = iamanager_v5_IAMOutgoingMessages_resume_node_response_tag;
    pbResumeNodeResponse = iamanager_v5_ResumeNodeResponse iamanager_v5_ResumeNodeResponse_init_default;

    NodeStatus expectedStatuses[] {NodeStatusEnum::ePaused};

    if (auto err = CheckNodeIDAndStatus(pbResumeNodeRequest.node_id, utils::ToArray(expectedStatuses)); !err.IsNone()) {
        return SendError(outgoingMessage.Get(), pbResumeNodeResponse, AOS_ERROR_WRAP(err));
    }

    if (auto err = mNodeInfoProvider->SetNodeStatus(NodeStatusEnum::eProvisioned); !err.IsNone()) {
        return SendError(outgoingMessage.Get(), pbResumeNodeResponse, AOS_ERROR_WRAP(err));
    }

    return SendMessage(outgoingMessage.Get(), &iamanager_v5_IAMOutgoingMessages_msg);
}

} // namespace aos::zephyr::iamclient
