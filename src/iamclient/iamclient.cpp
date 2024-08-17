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
    iam::provisionmanager::ProvisionManagerItf& provisionManager, communication::ChannelManagerItf& channelManager)
{
    LOG_DBG() << "Initialize IAM client";

    mProvisionManager = &provisionManager;

    if (auto err = clockSync.Subscribe(*this); !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    if (auto err = nodeInfoProvider.SubscribeNodeStatusChanged(*this); !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    NodeInfo nodeInfo;

    if (auto err = nodeInfoProvider.GetNodeInfo(nodeInfo); !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    mNodeID = nodeInfo.mNodeID;

    if (nodeInfo.mStatus == NodeStatusEnum::eUnprovisioned) {
        LOG_INF() << "Node is unprovisioned";

        auto channel = channelManager.CreateChannel(cOpenPort);
        if (!channel.mError.IsNone()) {
            return AOS_ERROR_WRAP(channel.mError);
        }

        if (auto err = PBHandler::Init("IAM open", *channel.mValue); !err.IsNone()) {
            return AOS_ERROR_WRAP(err);
        }

        if (auto err = Start(); !err.IsNone()) {
            return AOS_ERROR_WRAP(err);
        }
    }

    return ErrorEnum::eNone;
}

void IAMClient::OnClockSynced()
{
}

void IAMClient::OnClockUnsynced()
{
}

Error IAMClient::OnNodeStatusChanged(const String& nodeID, const NodeStatus& status)
{
    return ErrorEnum::eNone;
}

IAMClient::~IAMClient()
{
    Stop();
}

/***********************************************************************************************************************
 * Private
 **********************************************************************************************************************/

void IAMClient::OnConnect()
{
}

void IAMClient::OnDisconnect()
{
}

Error IAMClient::ReceiveMessage(const Array<uint8_t>& data)
{
    auto stream = pb_istream_from_buffer(data.Get(), data.Size());

    if (auto status = pb_decode(&stream, &iamanager_v5_IAMIncomingMessages_msg, &mIncomingMessages); !status) {
        return AOS_ERROR_WRAP(Error(ErrorEnum::eRuntime, "failed to decode message"));
    }

    Error err;

    switch (mIncomingMessages.which_IAMIncomingMessage) {
    case iamanager_v5_IAMIncomingMessages_start_provisioning_request_tag:
        err = ProcessStartProvisioning(mIncomingMessages.IAMIncomingMessage.start_provisioning_request);
        break;

    case iamanager_v5_IAMIncomingMessages_finish_provisioning_request_tag:
        err = ProcessFinishProvisioning(mIncomingMessages.IAMIncomingMessage.finish_provisioning_request);
        break;

    case iamanager_v5_IAMIncomingMessages_deprovision_request_tag:
        err = ProcessDeprovision(mIncomingMessages.IAMIncomingMessage.deprovision_request);
        break;

    case iamanager_v5_IAMIncomingMessages_get_cert_types_request_tag:
        err = ProcessGetCertTypes(mIncomingMessages.IAMIncomingMessage.get_cert_types_request);
        break;

    case iamanager_v5_IAMIncomingMessages_create_key_request_tag:
        err = ProcessCreateKey(mIncomingMessages.IAMIncomingMessage.create_key_request);
        break;

    case iamanager_v5_IAMIncomingMessages_apply_cert_request_tag:
        err = ProcessApplyCert(mIncomingMessages.IAMIncomingMessage.apply_cert_request);
        break;

    default:
        LOG_WRN() << "Receive unsupported message: tag=" << mIncomingMessages.which_IAMIncomingMessage;
        break;
    }

    return err;
}

template <typename T>
Error IAMClient::SendError(const Error& err, T& pbMessage)
{
    LOG_ERR() << "Process message error: err=" << err;

    utils::ErrorToPB(err, pbMessage);

    return SendMessage(&mOutgoingMessages, &iamanager_v5_IAMOutgoingMessages_msg);
}

Error IAMClient::ProcessStartProvisioning(const iamanager_v5_StartProvisioningRequest& pbStartProvisioningRequest)
{
    LOG_INF() << "Process start provisioning request";

    auto& pbStartProvisionResponse = mOutgoingMessages.IAMOutgoingMessage.start_provisioning_response;

    mOutgoingMessages.which_IAMOutgoingMessage = iamanager_v5_IAMOutgoingMessages_start_provisioning_response_tag;
    pbStartProvisionResponse
        = iamanager_v5_StartProvisioningResponse iamanager_v5_StartProvisioningResponse_init_default;

    if (pbStartProvisioningRequest.node_id != mNodeID) {
        return SendError(AOS_ERROR_WRAP(Error(ErrorEnum::eInvalidArgument, "wrong node ID")), pbStartProvisionResponse);
    }

    if (auto err = mProvisionManager->StartProvisioning(pbStartProvisioningRequest.password); !err.IsNone()) {
        return SendError(err, pbStartProvisionResponse);
    }

    return SendMessage(&mOutgoingMessages, &iamanager_v5_IAMOutgoingMessages_msg);
}

Error IAMClient::ProcessFinishProvisioning(const iamanager_v5_FinishProvisioningRequest& pbFinishProvisioningRequest)
{
    LOG_INF() << "Process finish provisioning request";

    auto& pbFinishProvisionResponse = mOutgoingMessages.IAMOutgoingMessage.finish_provisioning_response;

    mOutgoingMessages.which_IAMOutgoingMessage = iamanager_v5_IAMOutgoingMessages_finish_provisioning_response_tag;
    pbFinishProvisionResponse
        = iamanager_v5_FinishProvisioningResponse iamanager_v5_FinishProvisioningResponse_init_default;

    if (pbFinishProvisioningRequest.node_id != mNodeID) {
        return SendError(
            AOS_ERROR_WRAP(Error(ErrorEnum::eInvalidArgument, "wrong node ID")), pbFinishProvisionResponse);
    }

    if (auto err = mProvisionManager->FinishProvisioning(pbFinishProvisioningRequest.password); !err.IsNone()) {
        return SendError(err, pbFinishProvisionResponse);
    }

    return SendMessage(&mOutgoingMessages, &iamanager_v5_IAMOutgoingMessages_msg);
}

Error IAMClient::ProcessDeprovision(const iamanager_v5_DeprovisionRequest& pbDeprovisionRequest)
{
    LOG_INF() << "Process deprovision request";

    auto& pbDeprovisionResponse = mOutgoingMessages.IAMOutgoingMessage.deprovision_response;

    mOutgoingMessages.which_IAMOutgoingMessage = iamanager_v5_IAMOutgoingMessages_deprovision_response_tag;
    pbDeprovisionResponse = iamanager_v5_DeprovisionResponse iamanager_v5_DeprovisionResponse_init_default;

    if (pbDeprovisionRequest.node_id != mNodeID) {
        return SendError(AOS_ERROR_WRAP(Error(ErrorEnum::eInvalidArgument, "wrong node ID")), pbDeprovisionResponse);
    }

    if (auto err = mProvisionManager->Deprovision(pbDeprovisionRequest.password); !err.IsNone()) {
        return SendError(err, pbDeprovisionResponse);
    }

    return SendMessage(&mOutgoingMessages, &iamanager_v5_IAMOutgoingMessages_msg);
}

Error IAMClient::ProcessGetCertTypes(const iamanager_v5_GetCertTypesRequest& pbGetCertTypesRequest)
{
    LOG_INF() << "Process get cert types";

    auto& pbGetCertTypesResponse = mOutgoingMessages.IAMOutgoingMessage.cert_types_response;

    mOutgoingMessages.which_IAMOutgoingMessage = iamanager_v5_IAMOutgoingMessages_cert_types_response_tag;
    pbGetCertTypesResponse                     = iamanager_v5_CertTypes iamanager_v5_CertTypes_init_default;

    if (pbGetCertTypesRequest.node_id != mNodeID) {
        LOG_ERR() << "Wrong node ID: " << pbGetCertTypesRequest.node_id;
    }

    auto certTypes = mProvisionManager->GetCertTypes();
    if (!certTypes.mError.IsNone()) {
        LOG_ERR() << "Getting cert types error: err=" << certTypes.mError;
    }

    pbGetCertTypesResponse.types_count = certTypes.mValue.Size();

    for (size_t i = 0; i < certTypes.mValue.Size(); i++) {
        utils::StringFromCStr(pbGetCertTypesResponse.types[i]) = certTypes.mValue[i];
    }

    return SendMessage(&mOutgoingMessages, &iamanager_v5_IAMOutgoingMessages_msg);
}

Error IAMClient::ProcessCreateKey(const iamanager_v5_CreateKeyRequest& pbCreateKeyRequest)
{
    LOG_INF() << "Process create key: type=" << pbCreateKeyRequest.type << ", subject=" << pbCreateKeyRequest.subject;

    auto& pbCreateKeyResponse = mOutgoingMessages.IAMOutgoingMessage.create_key_response;

    mOutgoingMessages.which_IAMOutgoingMessage = iamanager_v5_IAMOutgoingMessages_create_key_response_tag;
    pbCreateKeyResponse = iamanager_v5_CreateKeyResponse iamanager_v5_CreateKeyResponse_init_default;

    if (pbCreateKeyRequest.node_id != mNodeID) {
        return SendError(AOS_ERROR_WRAP(Error(ErrorEnum::eInvalidArgument, "wrong node ID")), pbCreateKeyResponse);
    }

    auto csr = utils::StringFromCStr(pbCreateKeyResponse.csr);

    if (auto err = mProvisionManager->CreateKey(
            pbCreateKeyRequest.type, pbCreateKeyRequest.subject, pbCreateKeyRequest.password, csr);
        !err.IsNone()) {
        return SendError(err, pbCreateKeyResponse);
    }

    utils::StringFromCStr(pbCreateKeyResponse.type) = pbCreateKeyRequest.type;

    return SendMessage(&mOutgoingMessages, &iamanager_v5_IAMOutgoingMessages_msg);
}

Error IAMClient::ProcessApplyCert(const iamanager_v5_ApplyCertRequest& pbApplyCertRequest)
{
    LOG_INF() << "Process apply cert: type=" << pbApplyCertRequest.type;

    auto& pbApplyCertResponse = mOutgoingMessages.IAMOutgoingMessage.apply_cert_response;

    mOutgoingMessages.which_IAMOutgoingMessage = iamanager_v5_IAMOutgoingMessages_apply_cert_response_tag;
    pbApplyCertResponse = iamanager_v5_ApplyCertResponse iamanager_v5_ApplyCertResponse_init_default;

    if (pbApplyCertRequest.node_id != mNodeID) {
        return SendError(AOS_ERROR_WRAP(Error(ErrorEnum::eInvalidArgument, "wrong node ID")), pbApplyCertResponse);
    }

    iam::certhandler::CertInfo certInfo {};

    if (auto err = mProvisionManager->ApplyCert(pbApplyCertRequest.type, pbApplyCertRequest.cert, certInfo);
        !err.IsNone()) {
        return SendError(err, pbApplyCertResponse);
    }

    utils::StringFromCStr(pbApplyCertResponse.type)     = pbApplyCertRequest.type;
    utils::StringFromCStr(pbApplyCertResponse.cert_url) = certInfo.mCertURL;

    if (auto err = utils::StringFromCStr(pbApplyCertResponse.serial).ByteArrayToHex(certInfo.mSerial); !err.IsNone()) {
        return SendError(err, pbApplyCertResponse);
    }

    return SendMessage(&mOutgoingMessages, &iamanager_v5_IAMOutgoingMessages_msg);
}

} // namespace aos::zephyr::iamclient
