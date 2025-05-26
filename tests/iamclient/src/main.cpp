/*
 * Copyright (C) 2024 Renesas Electronics Corporation.
 * Copyright (C) 2024 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <memory>

#include <zephyr/tc_util.h>
#include <zephyr/ztest.h>

#include "iamclient/iamclient.hpp"

#include "stubs/channelmanagerstub.hpp"
#include "stubs/clocksyncstub.hpp"
#include "stubs/nodeinfoproviderstub.hpp"
#include "stubs/provisionmanagerstub.hpp"
#include "utils/log.hpp"
#include "utils/pbconvert.hpp"
#include "utils/pbmessages.hpp"
#include "utils/utils.hpp"

using namespace aos::zephyr;

/***********************************************************************************************************************
 * Consts
 **********************************************************************************************************************/

static constexpr auto cWaitTimeout = std::chrono::seconds {5};

/***********************************************************************************************************************
 * Types
 **********************************************************************************************************************/

struct iamclient_fixture {
    ClockSyncStub                         mClockSync;
    NodeInfoProviderStub                  mNodeInfoProvider;
    ProvisionManagerStub                  mProvisionManager;
    ChannelManagerStub                    mChannelManager;
    std::unique_ptr<iamclient::IAMClient> mIAMClient;
};

/***********************************************************************************************************************
 * Static
 **********************************************************************************************************************/

static aos::Error ReceiveIAMOutgoingMessage(ChannelStub* channel, iamanager_v5_IAMOutgoingMessages& message)
{
    return ReceivePBMessage(
        channel, cWaitTimeout, &message, iamanager_v5_IAMOutgoingMessages_size, &iamanager_v5_IAMOutgoingMessages_msg);
}

static aos::Error SendIAMIncomingMessage(ChannelStub* channel, const iamanager_v5_IAMIncomingMessages& message)
{
    return SendPBMessage(
        channel, &message, iamanager_v5_IAMIncomingMessages_size, &iamanager_v5_IAMIncomingMessages_msg);
}

static void InitIAMClient(iamclient_fixture* fixture, const aos::NodeInfo& nodeInfo)
{
    fixture->mNodeInfoProvider.SetNodeInfo(nodeInfo);

    auto err = fixture->mIAMClient->Init(
        fixture->mClockSync, fixture->mNodeInfoProvider, fixture->mProvisionManager, fixture->mChannelManager);
    zassert_true(err.IsNone(), "Can't initialize IAM client: %s", utils::ErrorToCStr(err));

    err = fixture->mIAMClient->Start();
    zassert_true(err.IsNone(), "Can't start IAM client: %s", utils::ErrorToCStr(err));

    fixture->mIAMClient->OnClockSynced();
}

static void ReceiveNodeInfo(ChannelStub* channel, const aos::NodeInfo& nodeInfo)
{
    iamanager_v5_IAMOutgoingMessages outgoingMessage;
    auto&                            pbNodeInfo = outgoingMessage.IAMOutgoingMessage.node_info;

    pbNodeInfo = iamanager_v5_NodeInfo iamanager_v5_NodeInfo_init_default;

    auto err = ReceiveIAMOutgoingMessage(channel, outgoingMessage);
    zassert_true(err.IsNone(), "Error receiving message: %s", utils::ErrorToCStr(err));

    zassert_equal(outgoingMessage.which_IAMOutgoingMessage, iamanager_v5_IAMOutgoingMessages_node_info_tag,
        "Unexpected message type");
    zassert_false(pbNodeInfo.has_error, "Unexpected error received");
    zassert_equal(pbNodeInfo.node_id, nodeInfo.mNodeID, "Wrong node ID");
    zassert_equal(pbNodeInfo.status, nodeInfo.mStatus.ToString(), "Wrong node status");
}

/***********************************************************************************************************************
 * Setup
 **********************************************************************************************************************/

ZTEST_SUITE(
    iamclient, nullptr,
    []() -> void* {
        aos::Log::SetCallback(TestLogCallback);

        auto fixture = new iamclient_fixture;

        return fixture;
    },
    [](void* fixture) { static_cast<iamclient_fixture*>(fixture)->mIAMClient.reset(new iamclient::IAMClient); },
    [](void* fixture) {
        auto& iamClientFixture = static_cast<iamclient_fixture*>(fixture)->mIAMClient;

        auto err = iamClientFixture->Stop();
        zassert_true(err.IsNone(), "Can't stop IAM client: %s", utils::ErrorToCStr(err));

        iamClientFixture.reset();
    },
    [](void* fixture) { delete static_cast<iamclient_fixture*>(fixture); });

/***********************************************************************************************************************
 * Tests
 **********************************************************************************************************************/

ZTEST_F(iamclient, test_StartProvisioning)
{
    aos::NodeInfo nodeInfo = {.mNodeID = "node1", .mStatus = aos::NodeStatusEnum::eUnprovisioned};

    InitIAMClient(fixture, nodeInfo);

    auto [channel, err] = fixture->mChannelManager.GetChannel(CONFIG_AOS_IAM_OPEN_PORT, cWaitTimeout);
    zassert_true(err.IsNone(), "Getting channel error: %s", utils::ErrorToCStr(err));

    ReceiveNodeInfo(channel, nodeInfo);

    // Send start provisioning request

    iamanager_v5_IAMIncomingMessages incomingMessage;
    auto& pbStartProvisioningRequest = incomingMessage.IAMIncomingMessage.start_provisioning_request;

    incomingMessage.which_IAMIncomingMessage = iamanager_v5_IAMIncomingMessages_start_provisioning_request_tag;
    pbStartProvisioningRequest
        = iamanager_v5_StartProvisioningRequest iamanager_v5_StartProvisioningRequest_init_default;

    utils::StringFromCStr(pbStartProvisioningRequest.node_id)  = nodeInfo.mNodeID;
    utils::StringFromCStr(pbStartProvisioningRequest.password) = "12345";

    err = SendIAMIncomingMessage(channel, incomingMessage);
    zassert_true(err.IsNone(), "Error sending message: %s", utils::ErrorToCStr(err));

    // Receive start provisioning response

    iamanager_v5_IAMOutgoingMessages outgoingMessage;
    auto& pbStartProvisioningResponse = outgoingMessage.IAMOutgoingMessage.start_provisioning_response;

    pbStartProvisioningResponse
        = iamanager_v5_StartProvisioningResponse iamanager_v5_StartProvisioningResponse_init_default;

    err = ReceiveIAMOutgoingMessage(channel, outgoingMessage);
    zassert_true(err.IsNone(), "Error receiving message: %s", utils::ErrorToCStr(err));

    zassert_equal(outgoingMessage.which_IAMOutgoingMessage,
        iamanager_v5_IAMOutgoingMessages_start_provisioning_response_tag, "Unexpected message type");
    zassert_false(pbStartProvisioningResponse.has_error, "Unexpected error received");

    zassert_equal(fixture->mProvisionManager.GetPassword(), pbStartProvisioningRequest.password, "Wrong password");
}

ZTEST_F(iamclient, test_FinishProvisioning)
{
    aos::NodeInfo nodeInfo = {.mNodeID = "node1", .mStatus = aos::NodeStatusEnum::eUnprovisioned};

    InitIAMClient(fixture, nodeInfo);

    auto [channel, err] = fixture->mChannelManager.GetChannel(CONFIG_AOS_IAM_OPEN_PORT, cWaitTimeout);
    zassert_true(err.IsNone(), "Getting channel error: %s", utils::ErrorToCStr(err));

    ReceiveNodeInfo(channel, nodeInfo);

    // Send finish provisioning request

    iamanager_v5_IAMIncomingMessages incomingMessage;
    auto& pbFinishProvisioningRequest = incomingMessage.IAMIncomingMessage.finish_provisioning_request;

    incomingMessage.which_IAMIncomingMessage = iamanager_v5_IAMIncomingMessages_finish_provisioning_request_tag;
    pbFinishProvisioningRequest
        = iamanager_v5_FinishProvisioningRequest iamanager_v5_FinishProvisioningRequest_init_default;

    utils::StringFromCStr(pbFinishProvisioningRequest.node_id)  = nodeInfo.mNodeID;
    utils::StringFromCStr(pbFinishProvisioningRequest.password) = "ABCDEF";

    err = SendIAMIncomingMessage(channel, incomingMessage);
    zassert_true(err.IsNone(), "Error sending message: %s", utils::ErrorToCStr(err));

    // Receive finish provisioning response

    iamanager_v5_IAMOutgoingMessages outgoingMessage;
    auto& pbFinishProvisioningResponse = outgoingMessage.IAMOutgoingMessage.finish_provisioning_response;

    pbFinishProvisioningResponse
        = iamanager_v5_FinishProvisioningResponse iamanager_v5_FinishProvisioningResponse_init_default;

    err = ReceiveIAMOutgoingMessage(channel, outgoingMessage);
    zassert_true(err.IsNone(), "Error receiving message: %s", utils::ErrorToCStr(err));

    zassert_equal(outgoingMessage.which_IAMOutgoingMessage,
        iamanager_v5_IAMOutgoingMessages_finish_provisioning_response_tag, "Unexpected message type");
    zassert_false(pbFinishProvisioningResponse.has_error, "Unexpected error received");

    zassert_equal(fixture->mProvisionManager.GetPassword(), pbFinishProvisioningRequest.password, "Wrong password");

    // Wait for node status changed

    aos::Tie(channel, err) = fixture->mChannelManager.GetChannel(CONFIG_AOS_IAM_SECURE_PORT, cWaitTimeout);
    zassert_true(err.IsNone(), "Getting channel error: %s", utils::ErrorToCStr(err));

    nodeInfo.mStatus = aos::NodeStatusEnum::eProvisioned;

    ReceiveNodeInfo(channel, nodeInfo);
}

ZTEST_F(iamclient, test_Deprovision)
{
    aos::NodeInfo nodeInfo = {.mNodeID = "node1", .mStatus = aos::NodeStatusEnum::eProvisioned};

    InitIAMClient(fixture, nodeInfo);

    auto [channel, err] = fixture->mChannelManager.GetChannel(CONFIG_AOS_IAM_SECURE_PORT, cWaitTimeout);
    zassert_true(err.IsNone(), "Getting channel error: %s", utils::ErrorToCStr(err));

    ReceiveNodeInfo(channel, nodeInfo);

    zassert_equal(nodeInfo.mStatus, aos::NodeStatusEnum::eProvisioned);

    // Send deprovision request

    iamanager_v5_IAMIncomingMessages incomingMessage;
    auto&                            pbDeprovisionRequest = incomingMessage.IAMIncomingMessage.deprovision_request;

    incomingMessage.which_IAMIncomingMessage = iamanager_v5_IAMIncomingMessages_deprovision_request_tag;
    pbDeprovisionRequest = iamanager_v5_DeprovisionRequest iamanager_v5_DeprovisionRequest_init_default;

    utils::StringFromCStr(pbDeprovisionRequest.node_id)  = nodeInfo.mNodeID;
    utils::StringFromCStr(pbDeprovisionRequest.password) = "FEDCBA";

    err = SendIAMIncomingMessage(channel, incomingMessage);
    zassert_true(err.IsNone(), "Error sending message: %s", utils::ErrorToCStr(err));

    // Receive deprovision response

    iamanager_v5_IAMOutgoingMessages outgoingMessage;
    auto&                            pbDeprovisionResponse = outgoingMessage.IAMOutgoingMessage.deprovision_response;

    pbDeprovisionResponse = iamanager_v5_DeprovisionResponse iamanager_v5_DeprovisionResponse_init_default;

    err = ReceiveIAMOutgoingMessage(channel, outgoingMessage);
    zassert_true(err.IsNone(), "Error receiving message: %s", utils::ErrorToCStr(err));

    zassert_equal(outgoingMessage.which_IAMOutgoingMessage, iamanager_v5_IAMOutgoingMessages_deprovision_response_tag,
        "Unexpected message type: type=%d", outgoingMessage.which_IAMOutgoingMessage);

    // Check that deprovision response has no error
    zassert_false(pbDeprovisionResponse.has_error, "Unexpected error received");

    zassert_equal(fixture->mProvisionManager.GetPassword(), pbDeprovisionRequest.password, "Wrong password");

    // Wait for node status changed

    aos::Tie(channel, err) = fixture->mChannelManager.GetChannel(CONFIG_AOS_IAM_OPEN_PORT, cWaitTimeout);
    zassert_true(err.IsNone(), "Getting channel error: %s", utils::ErrorToCStr(err));

    nodeInfo.mStatus = aos::NodeStatusEnum::eUnprovisioned;

    ReceiveNodeInfo(channel, nodeInfo);
}

ZTEST_F(iamclient, test_GetCertTypes)
{
    aos::NodeInfo nodeInfo = {.mNodeID = "node1", .mStatus = aos::NodeStatusEnum::eUnprovisioned};

    InitIAMClient(fixture, nodeInfo);

    auto [channel, err] = fixture->mChannelManager.GetChannel(CONFIG_AOS_IAM_OPEN_PORT, cWaitTimeout);
    zassert_true(err.IsNone(), "Getting channel error: %s", utils::ErrorToCStr(err));

    ReceiveNodeInfo(channel, nodeInfo);

    aos::iam::provisionmanager::CertTypes certTypes;

    certTypes.PushBack("certType1");
    certTypes.PushBack("certType2");

    fixture->mProvisionManager.SetCertTypes(certTypes);

    // Send get cert types request

    iamanager_v5_IAMIncomingMessages incomingMessage;
    auto&                            pbGetCertTypesRequest = incomingMessage.IAMIncomingMessage.get_cert_types_request;

    incomingMessage.which_IAMIncomingMessage = iamanager_v5_IAMIncomingMessages_get_cert_types_request_tag;
    pbGetCertTypesRequest = iamanager_v5_GetCertTypesRequest iamanager_v5_GetCertTypesRequest_init_default;

    utils::StringFromCStr(pbGetCertTypesRequest.node_id) = nodeInfo.mNodeID;

    err = SendIAMIncomingMessage(channel, incomingMessage);
    zassert_true(err.IsNone(), "Error sending message: %s", utils::ErrorToCStr(err));

    // Receive get cert types response

    iamanager_v5_IAMOutgoingMessages outgoingMessage;
    auto&                            pbGetCertTypesResponse = outgoingMessage.IAMOutgoingMessage.cert_types_response;

    pbGetCertTypesResponse = iamanager_v5_CertTypes iamanager_v5_CertTypes_init_default;

    err = ReceiveIAMOutgoingMessage(channel, outgoingMessage);
    zassert_true(err.IsNone(), "Error receiving message: %s", utils::ErrorToCStr(err));

    zassert_equal(outgoingMessage.which_IAMOutgoingMessage, iamanager_v5_IAMOutgoingMessages_cert_types_response_tag,
        "Unexpected message type");
    zassert_equal(pbGetCertTypesResponse.types_count, certTypes.Size(), "Wrong cert types count");

    for (size_t i = 0; i < certTypes.Size(); i++) {
        zassert_equal(pbGetCertTypesResponse.types[i], certTypes[i], "Wrong cert type");
    }
}

ZTEST_F(iamclient, test_CreateKey)
{
    aos::NodeInfo nodeInfo = {.mNodeID = "node1", .mStatus = aos::NodeStatusEnum::eUnprovisioned};

    InitIAMClient(fixture, nodeInfo);

    auto [channel, err] = fixture->mChannelManager.GetChannel(CONFIG_AOS_IAM_OPEN_PORT, cWaitTimeout);
    zassert_true(err.IsNone(), "Getting channel error: %s", utils::ErrorToCStr(err));

    ReceiveNodeInfo(channel, nodeInfo);

    fixture->mProvisionManager.SetCSR("csr1");

    // Send create key request

    iamanager_v5_IAMIncomingMessages incomingMessage;
    auto&                            pbCreateKeyRequest = incomingMessage.IAMIncomingMessage.create_key_request;

    incomingMessage.which_IAMIncomingMessage = iamanager_v5_IAMIncomingMessages_create_key_request_tag;
    pbCreateKeyRequest                       = iamanager_v5_CreateKeyRequest iamanager_v5_CreateKeyRequest_init_default;

    utils::StringFromCStr(pbCreateKeyRequest.node_id)  = nodeInfo.mNodeID;
    utils::StringFromCStr(pbCreateKeyRequest.type)     = "certType1";
    utils::StringFromCStr(pbCreateKeyRequest.subject)  = "subject1";
    utils::StringFromCStr(pbCreateKeyRequest.password) = "54321";

    err = SendIAMIncomingMessage(channel, incomingMessage);
    zassert_true(err.IsNone(), "Error sending message: %s", utils::ErrorToCStr(err));

    // Receive create key response

    iamanager_v5_IAMOutgoingMessages outgoingMessage;
    auto&                            pbCreateKeyResponse = outgoingMessage.IAMOutgoingMessage.create_key_response;

    pbCreateKeyResponse = iamanager_v5_CreateKeyResponse iamanager_v5_CreateKeyResponse_init_default;

    err = ReceiveIAMOutgoingMessage(channel, outgoingMessage);
    zassert_true(err.IsNone(), "Error receiving message: %s", utils::ErrorToCStr(err));

    zassert_equal(outgoingMessage.which_IAMOutgoingMessage, iamanager_v5_IAMOutgoingMessages_create_key_response_tag,
        "Unexpected message type");
    zassert_false(pbCreateKeyResponse.has_error, "Unexpected error received");
    zassert_equal(utils::StringFromCStr(pbCreateKeyResponse.type), utils::StringFromCStr(pbCreateKeyRequest.type),
        "Wrong cert type");
    zassert_equal(pbCreateKeyResponse.csr, fixture->mProvisionManager.GetCSR(), "Wrong CSR");
    zassert_equal(pbCreateKeyRequest.type, fixture->mProvisionManager.GetCertType(), "Wrong cert type");
    zassert_equal(pbCreateKeyRequest.subject, fixture->mProvisionManager.GetSubject(), "Wrong subject");
    zassert_equal(pbCreateKeyRequest.password, fixture->mProvisionManager.GetPassword(), "Wrong password");
}

ZTEST_F(iamclient, test_ApplyCert)
{
    aos::NodeInfo nodeInfo = {.mNodeID = "node1", .mStatus = aos::NodeStatusEnum::eUnprovisioned};

    InitIAMClient(fixture, nodeInfo);

    auto [channel, err] = fixture->mChannelManager.GetChannel(CONFIG_AOS_IAM_OPEN_PORT, cWaitTimeout);
    zassert_true(err.IsNone(), "Getting channel error: %s", utils::ErrorToCStr(err));

    ReceiveNodeInfo(channel, nodeInfo);

    aos::iam::certhandler::CertInfo certInfo {};

    aos::StaticString<aos::crypto::cSerialNumStrLen> serial = "0123456789abcdef";

    err = serial.HexToByteArray(certInfo.mSerial);
    zassert_true(err.IsNone(), "Convert serial error: %s", utils::ErrorToCStr(err));

    fixture->mProvisionManager.SetCertInfo(certInfo);

    // Send apply cert request

    iamanager_v5_IAMIncomingMessages incomingMessage;
    auto&                            pbApplyCertRequest = incomingMessage.IAMIncomingMessage.apply_cert_request;

    incomingMessage.which_IAMIncomingMessage = iamanager_v5_IAMIncomingMessages_apply_cert_request_tag;
    pbApplyCertRequest                       = iamanager_v5_ApplyCertRequest iamanager_v5_ApplyCertRequest_init_default;

    utils::StringFromCStr(pbApplyCertRequest.node_id) = nodeInfo.mNodeID;
    utils::StringFromCStr(pbApplyCertRequest.type)    = "certType2";
    utils::StringFromCStr(pbApplyCertRequest.cert)    = "cert1";

    err = SendIAMIncomingMessage(channel, incomingMessage);
    zassert_true(err.IsNone(), "Error sending message: %s", utils::ErrorToCStr(err));

    // Receive apply cert response

    iamanager_v5_IAMOutgoingMessages outgoingMessage;
    auto&                            pbApplyCertResponse = outgoingMessage.IAMOutgoingMessage.apply_cert_response;

    pbApplyCertResponse = iamanager_v5_ApplyCertResponse iamanager_v5_ApplyCertResponse_init_default;

    err = ReceiveIAMOutgoingMessage(channel, outgoingMessage);
    zassert_true(err.IsNone(), "Error receiving message: %s", utils::ErrorToCStr(err));

    zassert_equal(outgoingMessage.which_IAMOutgoingMessage, iamanager_v5_IAMOutgoingMessages_apply_cert_response_tag,
        "Unexpected message type");
    zassert_false(pbApplyCertResponse.has_error, "Unexpected error received");
    zassert_equal(pbApplyCertResponse.type, fixture->mProvisionManager.GetCertType(), "Wrong cert type");
    zassert_equal(pbApplyCertResponse.cert_url, certInfo.mCertURL, "Wrong cert URL");
    zassert_equal(pbApplyCertResponse.serial, serial, "Wrong serial");
    zassert_equal(pbApplyCertRequest.cert, fixture->mProvisionManager.GetCert(), "Wrong cert");
}

ZTEST_F(iamclient, test_PauseNode)
{
    aos::NodeInfo nodeInfo = {.mNodeID = "node1", .mStatus = aos::NodeStatusEnum::eProvisioned};

    InitIAMClient(fixture, nodeInfo);

    auto [channel, err] = fixture->mChannelManager.GetChannel(CONFIG_AOS_IAM_SECURE_PORT, cWaitTimeout);
    zassert_true(err.IsNone(), "Getting channel error: %s", utils::ErrorToCStr(err));

    ReceiveNodeInfo(channel, nodeInfo);

    // Send pause node request

    iamanager_v5_IAMIncomingMessages incomingMessage;
    auto&                            pbPauseNodeRequest = incomingMessage.IAMIncomingMessage.pause_node_request;

    incomingMessage.which_IAMIncomingMessage = iamanager_v5_IAMIncomingMessages_pause_node_request_tag;
    pbPauseNodeRequest                       = iamanager_v5_PauseNodeRequest iamanager_v5_PauseNodeRequest_init_default;

    utils::StringFromCStr(pbPauseNodeRequest.node_id) = nodeInfo.mNodeID;

    err = SendIAMIncomingMessage(channel, incomingMessage);
    zassert_true(err.IsNone(), "Error sending message: %s", utils::ErrorToCStr(err));

    // Receive node info

    nodeInfo.mStatus = aos::NodeStatusEnum::ePaused;

    ReceiveNodeInfo(channel, nodeInfo);

    // Receive pause node response

    iamanager_v5_IAMOutgoingMessages outgoingMessage;
    auto&                            pbPauseNodeResponse = outgoingMessage.IAMOutgoingMessage.pause_node_response;

    pbPauseNodeResponse = iamanager_v5_PauseNodeResponse iamanager_v5_PauseNodeResponse_init_default;

    err = ReceiveIAMOutgoingMessage(channel, outgoingMessage);
    zassert_true(err.IsNone(), "Error receiving message: %s", utils::ErrorToCStr(err));

    zassert_equal(outgoingMessage.which_IAMOutgoingMessage, iamanager_v5_IAMOutgoingMessages_pause_node_response_tag,
        "Unexpected message type");
    zassert_false(pbPauseNodeResponse.has_error, "Unexpected error received");
}

ZTEST_F(iamclient, test_ResumeNode)
{
    aos::NodeInfo nodeInfo = {.mNodeID = "node1", .mStatus = aos::NodeStatusEnum::ePaused};

    InitIAMClient(fixture, nodeInfo);

    auto [channel, err] = fixture->mChannelManager.GetChannel(CONFIG_AOS_IAM_SECURE_PORT, cWaitTimeout);
    zassert_true(err.IsNone(), "Getting channel error: %s", utils::ErrorToCStr(err));

    ReceiveNodeInfo(channel, nodeInfo);

    // Send resume node request

    iamanager_v5_IAMIncomingMessages incomingMessage;
    auto&                            pbResumeNodeRequest = incomingMessage.IAMIncomingMessage.resume_node_request;

    incomingMessage.which_IAMIncomingMessage = iamanager_v5_IAMIncomingMessages_resume_node_request_tag;
    pbResumeNodeRequest = iamanager_v5_ResumeNodeRequest iamanager_v5_ResumeNodeRequest_init_default;

    utils::StringFromCStr(pbResumeNodeRequest.node_id) = nodeInfo.mNodeID;

    err = SendIAMIncomingMessage(channel, incomingMessage);
    zassert_true(err.IsNone(), "Error sending message: %s", utils::ErrorToCStr(err));

    // Receive node info

    nodeInfo.mStatus = aos::NodeStatusEnum::eProvisioned;

    ReceiveNodeInfo(channel, nodeInfo);

    // Receive resume node response

    iamanager_v5_IAMOutgoingMessages outgoingMessage;
    auto&                            pbResumeNodeResponse = outgoingMessage.IAMOutgoingMessage.resume_node_response;

    pbResumeNodeResponse = iamanager_v5_ResumeNodeResponse iamanager_v5_ResumeNodeResponse_init_default;

    err = ReceiveIAMOutgoingMessage(channel, outgoingMessage);
    zassert_true(err.IsNone(), "Error receiving message: %s", utils::ErrorToCStr(err));

    zassert_equal(outgoingMessage.which_IAMOutgoingMessage, iamanager_v5_IAMOutgoingMessages_resume_node_response_tag,
        "Unexpected message type");
    zassert_false(pbResumeNodeResponse.has_error, "Unexpected error received");
}
