/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/tc_util.h>
#include <zephyr/ztest.h>

#include "communication/communication.hpp"

#include "mocks/certhandlermock.hpp"
#include "mocks/clocksyncmock.hpp"
#include "mocks/commchannelmock.hpp"
#include "mocks/connectionsubscribermock.hpp"
#include "mocks/downloadermock.hpp"
#include "mocks/launchermock.hpp"
#include "mocks/monitoringmock.hpp"
#include "mocks/resourcemanagermock.hpp"
#include "utils.hpp"

/***********************************************************************************************************************
 * Types
 **********************************************************************************************************************/

struct iamserver_fixture {
    CommChannelMock     mOpenChannel;
    CommChannelMock     mSecureChannel;
    LauncherMock        mLauncher;
    CertHandlerMock     mCertHandler;
    ResourceManagerMock mResourceManager;
    ResourceMonitorMock mResourceMonitor;
    DownloaderMock      mDownloader;
    ClockSyncMock       mClockSync;
    Communication       mCommunication;
};

/***********************************************************************************************************************
 * Static
 **********************************************************************************************************************/

static std::string FullMethodName(const std::string& serviceName, const std::string& methodName)
{
    return "/" + serviceName + "/" + methodName;
}

/***********************************************************************************************************************
 * Setup
 **********************************************************************************************************************/

ZTEST_SUITE(
    iamserver, nullptr,
    []() -> void* {
        aos::Log::SetCallback(TestLogCallback);

        auto fixture = new iamserver_fixture;

        auto err = fixture->mCommunication.Init(fixture->mOpenChannel, fixture->mSecureChannel, fixture->mLauncher,
            fixture->mCertHandler, fixture->mResourceManager, fixture->mResourceMonitor, fixture->mDownloader,
            fixture->mClockSync);
        zassert_true(err.IsNone(), "Can't initialize communication: %s", err.Message());

        fixture->mCommunication.ClockSynced();

        return fixture;
    },
    [](void* fixture) {
        auto f = static_cast<iamserver_fixture*>(fixture);

        f->mOpenChannel.Clear();
        f->mSecureChannel.Clear();
        f->mLauncher.Clear();
        f->mCertHandler.Clear();
        f->mResourceManager.Clear();
        f->mResourceMonitor.Clear();
        f->mDownloader.Clear();
        f->mClockSync.Clear();
    },
    nullptr, [](void* fixture) { delete static_cast<iamserver_fixture*>(fixture); });

/***********************************************************************************************************************
 * Tests
 **********************************************************************************************************************/

ZTEST_F(iamserver, test_GetAllNodeIDs)
{
    auto err = SendPBMessage(fixture->mOpenChannel, AOS_VCHAN_IAM,
        FullMethodName(IAMServer::cServiceProvisioning, IAMServer::cMethodGetAllNodeIDs), 0);
    zassert_true(err.IsNone(), "Error sending message: %s", err.Message());

    iamanager_v4_NodesID nodesID;

    err = ReceivePBMessage(
        fixture->mOpenChannel, AOS_VCHAN_IAM, "", 0, &iamanager_v4_NodesID_msg, &nodesID, iamanager_v4_NodesID_size);
    zassert_true(err.IsNone(), "Error receiving message: %s", err.Message());

    zassert_equal(nodesID.ids_count, 1);
    zassert_equal(strcmp(nodesID.ids[0], CONFIG_AOS_NODE_ID), 0);
}

ZTEST_F(iamserver, test_GetCertTypes)
{
    std::vector<std::string> certTypes {"certType1", "certType2", "certType3"};

    fixture->mCertHandler.SetCertTypes(certTypes);

    auto err = SendPBMessage(fixture->mOpenChannel, AOS_VCHAN_IAM,
        FullMethodName(IAMServer::cServiceProvisioning, IAMServer::cMethodGetCertTypes), 0);
    zassert_true(err.IsNone(), "Error sending message: %s", err.Message());

    iamanager_v4_CertTypes pbCertTypes;

    err = ReceivePBMessage(fixture->mOpenChannel, AOS_VCHAN_IAM, "", 0, &iamanager_v4_CertTypes_msg, &pbCertTypes,
        iamanager_v4_CertTypes_size);
    zassert_true(err.IsNone(), "Error receiving message: %s", err.Message());

    zassert_equal(pbCertTypes.types_count, certTypes.size());

    for (size_t i = 0; i < certTypes.size(); i++) {
        zassert_equal(strcmp(certTypes[i].c_str(), pbCertTypes.types[i]), 0);
    }
}

ZTEST_F(iamserver, test_SetOwner)
{
    iamanager_v4_SetOwnerRequest pbSetOwner {CONFIG_AOS_NODE_ID, "certType", "password"};

    auto err = SendPBMessage(fixture->mOpenChannel, AOS_VCHAN_IAM,
        FullMethodName(IAMServer::cServiceProvisioning, IAMServer::cMethodSetOwner), 42,
        &iamanager_v4_SetOwnerRequest_msg, &pbSetOwner, iamanager_v4_SetOwnerRequest_size);
    zassert_true(err.IsNone(), "Error sending message: %s", err.Message());

    err = ReceivePBMessage(fixture->mOpenChannel, AOS_VCHAN_IAM, "", 42);
    zassert_true(err.IsNone(), "Error receiving message: %s", err.Message());

    zassert_equal(fixture->mCertHandler.GetCertType(), pbSetOwner.type);
    zassert_equal(fixture->mCertHandler.GetPassword(), pbSetOwner.password);
}

ZTEST_F(iamserver, test_Clear)
{
    iamanager_v4_ClearRequest pbClear {CONFIG_AOS_NODE_ID, "certType"};

    auto err = SendPBMessage(fixture->mOpenChannel, AOS_VCHAN_IAM,
        FullMethodName(IAMServer::cServiceProvisioning, IAMServer::cMethodClear), 43, &iamanager_v4_ClearRequest_msg,
        &pbClear, iamanager_v4_ClearRequest_size);
    zassert_true(err.IsNone(), "Error sending message: %s", err.Message());

    err = ReceivePBMessage(fixture->mOpenChannel, AOS_VCHAN_IAM, "", 43);
    zassert_true(err.IsNone(), "Error receiving message: %s", err.Message());

    zassert_true(fixture->mCertHandler.GetClear());
}

ZTEST_F(iamserver, test_DiskEncryption)
{
    iamanager_v4_EncryptDiskRequest pbEncryptDisk {CONFIG_AOS_NODE_ID, "password"};

    auto err = SendPBMessage(fixture->mOpenChannel, AOS_VCHAN_IAM,
        FullMethodName(IAMServer::cServiceProvisioning, IAMServer::cMethodEncryptDisk), 44,
        &iamanager_v4_EncryptDiskRequest_msg, &pbEncryptDisk, iamanager_v4_EncryptDiskRequest_size);
    zassert_true(err.IsNone(), "Error sending message: %s", err.Message());

    err = ReceivePBMessage(fixture->mOpenChannel, AOS_VCHAN_IAM, "", 44);
    zassert_true(err.Is(aos::ErrorEnum::eNotSupported), "Wrong error received: %s", err.Message());
}

ZTEST_F(iamserver, test_FinishProvisioning)
{
    auto err = SendPBMessage(fixture->mOpenChannel, AOS_VCHAN_IAM,
        FullMethodName(IAMServer::cServiceProvisioning, IAMServer::cMethodFinishProvisioning), 45);
    zassert_true(err.IsNone(), "Error sending message: %s", err.Message());

    err = ReceivePBMessage(fixture->mOpenChannel, AOS_VCHAN_IAM, "", 45);
    zassert_true(err.IsNone(), "Error receiving message: %s", err.Message());
}

ZTEST_F(iamserver, test_CreateKey)
{
    iamanager_v4_CreateKeyRequest pbCreateKeyRequest {CONFIG_AOS_NODE_ID, "subject", "type", "password"};

    const auto csr = "CSR payload";

    fixture->mCertHandler.SetCSR(csr);

    auto err = SendPBMessage(fixture->mOpenChannel, AOS_VCHAN_IAM,
        FullMethodName(IAMServer::cServiceCertificate, IAMServer::cMethodCreateKey), 46,
        &iamanager_v4_CreateKeyRequest_msg, &pbCreateKeyRequest, iamanager_v4_CreateKeyRequest_size);
    zassert_true(err.IsNone(), "Error sending message: %s", err.Message());

    iamanager_v4_CreateKeyResponse pbCreateKeyResponse iamanager_v4_CreateKeyResponse_init_default;

    err = ReceivePBMessage(fixture->mOpenChannel, AOS_VCHAN_IAM, "", 46, &iamanager_v4_CreateKeyResponse_msg,
        &pbCreateKeyResponse, iamanager_v4_CreateKeyResponse_size);
    zassert_true(err.IsNone(), "Error receiving message: %s", err.Message());

    zassert_equal(strcmp(pbCreateKeyResponse.node_id, pbCreateKeyRequest.node_id), 0);
    zassert_equal(strcmp(pbCreateKeyResponse.type, pbCreateKeyRequest.type), 0);
    zassert_equal(strcmp(pbCreateKeyResponse.csr, csr), 0);
    zassert_equal(fixture->mCertHandler.GetSubject(), pbCreateKeyRequest.subject);
}

ZTEST_F(iamserver, test_ApplyCert)
{
    iamanager_v4_ApplyCertRequest pbApplyCertRequest {CONFIG_AOS_NODE_ID, "type", "Certificate payload"};

    const uint8_t serial[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

    aos::iam::certhandler::CertInfo certInfo {{}, aos::Array<uint8_t>(serial, sizeof(serial)), "certificate URL"};

    fixture->mCertHandler.SetCertInfo(certInfo);

    auto err = SendPBMessage(fixture->mOpenChannel, AOS_VCHAN_IAM,
        FullMethodName(IAMServer::cServiceCertificate, IAMServer::cMethodApplyCert), 47,
        &iamanager_v4_ApplyCertRequest_msg, &pbApplyCertRequest, iamanager_v4_ApplyCertRequest_size);
    zassert_true(err.IsNone(), "Error sending message: %s", err.Message());

    iamanager_v4_ApplyCertResponse pbApplyCertResponse iamanager_v4_ApplyCertResponse_init_default;

    err = ReceivePBMessage(fixture->mOpenChannel, AOS_VCHAN_IAM, "", 47, &iamanager_v4_ApplyCertResponse_msg,
        &pbApplyCertResponse, iamanager_v4_ApplyCertResponse_size);
    zassert_true(err.IsNone(), "Error receiving message: %s", err.Message());

    aos::StaticString<aos::crypto::cSerialNumStrLen> strSerial;

    err = strSerial.ByteArrayToHex(aos::Array<uint8_t>(serial, sizeof(serial)));
    zassert_true(err.IsNone(), "Can't convert serial: %s", err.Message());

    zassert_equal(strcmp(pbApplyCertResponse.node_id, pbApplyCertRequest.node_id), 0);
    zassert_equal(strcmp(pbApplyCertResponse.type, pbApplyCertRequest.type), 0);
    zassert_equal(fixture->mCertHandler.GetCertificate(), pbApplyCertRequest.cert);
    zassert_equal(certInfo.mCertURL, pbApplyCertResponse.cert_url);
    zassert_equal(strSerial, pbApplyCertResponse.serial);
}
