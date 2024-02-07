/*
 * Copyright (C) 2024 Renesas Electronics Corporation.
 * Copyright (C) 2024 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <pb_decode.h>
#include <pb_encode.h>

#include "utils/pbconvert.hpp"

#include "iamserver.hpp"
#include "log.hpp"

/***********************************************************************************************************************
 * Public
 **********************************************************************************************************************/

aos::Error IAMServer::Init(
    aos::iam::certhandler::CertHandlerItf& certHandler, ProvisioningItf& provisioning, MessageSenderItf& messageSender)
{
    LOG_DBG() << "Initialize IAM server";

    auto err = MessageHandler::Init(AOS_VCHAN_IAM, messageSender);
    if (!err.IsNone()) {
        return err;
    }

    mCertHandler   = &certHandler;
    mProvisioning  = &provisioning;
    mMessageSender = &messageSender;

    aos::StaticString<cMaxServiceLen + cMaxMethodLen> fullMethodName;

    // Provisioning service

    if (!(err = mProvisioningService.Init(cServiceProvisioning)).IsNone()) {
        return err;
    }

    mProvisioningService.RegisterMethod(cMethodGetAllNodeIDs);
    mProvisioningService.RegisterMethod(cMethodGetCertTypes);
    mProvisioningService.RegisterMethod(cMethodSetOwner);
    mProvisioningService.RegisterMethod(cMethodClear);
    mProvisioningService.RegisterMethod(cMethodEncryptDisk);
    mProvisioningService.RegisterMethod(cMethodFinishProvisioning);

    mProvisioningService.GetFullMethodName(cMethodGetAllNodeIDs, fullMethodName);
    mHandlers.EmplaceBack(MethodHandler {fullMethodName, &IAMServer::ProcessGetAllNodeIDs});
    mProvisioningService.GetFullMethodName(cMethodGetCertTypes, fullMethodName);
    mHandlers.EmplaceBack(MethodHandler {fullMethodName, &IAMServer::ProcessGetGetCertTypes});
    mProvisioningService.GetFullMethodName(cMethodSetOwner, fullMethodName);
    mHandlers.EmplaceBack(MethodHandler {fullMethodName, &IAMServer::ProcessSetOwner});
    mProvisioningService.GetFullMethodName(cMethodClear, fullMethodName);
    mHandlers.EmplaceBack(MethodHandler {fullMethodName, &IAMServer::ProcessClear});
    mProvisioningService.GetFullMethodName(cMethodEncryptDisk, fullMethodName);
    mHandlers.EmplaceBack(MethodHandler {fullMethodName, &IAMServer::ProcessEncryptDisk});
    mProvisioningService.GetFullMethodName(cMethodFinishProvisioning, fullMethodName);
    mHandlers.EmplaceBack(MethodHandler {fullMethodName, &IAMServer::ProcessFinishProvisioning});

    // Certificate service

    if (!(err = mCertificateService.Init(cServiceCertificate)).IsNone()) {
        return err;
    }

    mCertificateService.RegisterMethod(cMethodCreateKey);
    mCertificateService.RegisterMethod(cMethodApplyCert);

    mCertificateService.GetFullMethodName(cMethodCreateKey, fullMethodName);
    mHandlers.EmplaceBack(MethodHandler {fullMethodName, &IAMServer::ProcessCreateKey});
    mCertificateService.GetFullMethodName(cMethodApplyCert, fullMethodName);
    mHandlers.EmplaceBack(MethodHandler {fullMethodName, &IAMServer::ProcessApplyCert});

    // Register services

    if (!mProvisioning->IsProvisioned()) {
        if (!(err = RegisterService(mProvisioningService, ChannelEnum::eSecure)).IsNone()) {
            return err;
        }
    }

    if (!(err = RegisterService(mCertificateService, ChannelEnum::eSecure)).IsNone()) {
        return err;
    }

    return aos::ErrorEnum::eNone;
}

/***********************************************************************************************************************
 * Private
 **********************************************************************************************************************/

aos::Error IAMServer::ProcessMessage(Channel channel, PBServiceItf& service, const aos::String& methodName,
    uint64_t requestID, const aos::Array<uint8_t>& data)
{
    LOG_DBG() << "Receive IAM message: "
              << "service = " << service.GetServiceName() << ", method = " << methodName;

    aos::StaticString<cMaxServiceLen + cMaxMethodLen> fullMethodName;

    auto err = service.GetFullMethodName(methodName, fullMethodName);
    if (!err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    auto methodHandler
        = mHandlers.Find([&fullMethodName](const auto& item) { return item.mMethodName == fullMethodName; });
    if (!methodHandler.mError.IsNone()) {
        return AOS_ERROR_WRAP(methodHandler.mError);
    }

    return (this->*methodHandler.mValue->mCallback)(channel, requestID, data);
}

aos::Error IAMServer::ProcessGetAllNodeIDs(Channel channel, uint64_t requestID, const aos::Array<uint8_t>& data)
{
    iamanager_v4_NodesID nodesID iamanager_v4_NodesID_init_default;

    nodesID.ids_count = 1;
    StringToPB(cNodeID, nodesID.ids[0]);

    auto err = SendPBMessage(channel, requestID, &iamanager_v4_NodesID_msg, &nodesID);
    if (!err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    return aos::ErrorEnum::eNone;
}

aos::Error IAMServer::ProcessGetGetCertTypes(Channel channel, uint64_t requestID, const aos::Array<uint8_t>& data)
{
    aos::StaticArray<aos::StaticString<aos::iam::certhandler::cCertTypeLen>,
        aos::iam::certhandler::cIAMCertModulesMaxCount>
                                       aosCertTypes;
    iamanager_v4_CertTypes pbCertTypes iamanager_v4_CertTypes_init_default;

    auto err = mCertHandler->GetCertTypes(aosCertTypes);
    if (err.IsNone()) {
        pbCertTypes.types_count = aosCertTypes.Size();

        for (size_t i = 0; i < aosCertTypes.Size(); i++) {
            StringToPB(aosCertTypes[i], pbCertTypes.types[i]);
        }
    }

    auto sendErr = SendPBMessage(channel, requestID, &iamanager_v4_CertTypes_msg, &pbCertTypes, err);
    if (!sendErr.IsNone() && err.IsNone()) {
        err = sendErr;
    }

    return AOS_ERROR_WRAP(err);
}

aos::Error IAMServer::ProcessSetOwner(Channel channel, uint64_t requestID, const aos::Array<uint8_t>& data)
{
    iamanager_v4_SetOwnerRequest pbSetOwner iamanager_v4_SetOwnerRequest_init_default;

    auto err = DecodePBMessage(data, &iamanager_v4_SetOwnerRequest_msg, &pbSetOwner);

    if (err.IsNone()) {
        err = CheckPBNodeID(pbSetOwner.node_id);
    }

    if (err.IsNone()) {
        err = mCertHandler->SetOwner(pbSetOwner.type, pbSetOwner.password);
    }

    auto sendErr = SendPBMessage(channel, requestID, nullptr, nullptr, err);
    if (!sendErr.IsNone() && err.IsNone()) {
        err = sendErr;
    }

    return AOS_ERROR_WRAP(err);
}

aos::Error IAMServer::ProcessClear(Channel channel, uint64_t requestID, const aos::Array<uint8_t>& data)
{
    iamanager_v4_ClearRequest pbClear iamanager_v4_ClearRequest_init_default;

    auto err = DecodePBMessage(data, &iamanager_v4_ClearRequest_msg, &pbClear);

    if (err.IsNone()) {
        err = CheckPBNodeID(pbClear.node_id);
    }

    if (err.IsNone()) {
        err = mCertHandler->Clear(pbClear.type);
    }

    auto sendErr = SendPBMessage(channel, requestID, nullptr, nullptr, err);
    if (!sendErr.IsNone() && err.IsNone()) {
        err = sendErr;
    }

    return AOS_ERROR_WRAP(err);
}

aos::Error IAMServer::ProcessEncryptDisk(Channel channel, uint64_t requestID, const aos::Array<uint8_t>& data)
{
    iamanager_v4_EncryptDiskRequest pbEncryptDisk iamanager_v4_EncryptDiskRequest_init_default;

    auto err = DecodePBMessage(data, &iamanager_v4_EncryptDiskRequest_msg, &pbEncryptDisk);

    if (err.IsNone()) {
        err = CheckPBNodeID(pbEncryptDisk.node_id);
    }

    if (err.IsNone()) {
        err = AOS_ERROR_WRAP(aos::ErrorEnum::eNotSupported);

        LOG_ERR() << "Disk encryption error: " << err;
    }

    auto sendErr = SendPBMessage(channel, requestID, nullptr, nullptr, err);
    if (!sendErr.IsNone() && err.IsNone()) {
        err = sendErr;
    }

    return AOS_ERROR_WRAP(err);
}

aos::Error IAMServer::ProcessFinishProvisioning(Channel channel, uint64_t requestID, const aos::Array<uint8_t>& data)
{
    aos::Error messageError;

    auto err = RemoveService(mProvisioningService, ChannelEnum::eSecure);
    if (!err.IsNone() && !messageError.IsNone()) {
        messageError = err;
    }

    err = mProvisioning->FinishProvisioning();
    if (!err.IsNone() && !messageError.IsNone()) {
        messageError = err;
    }

    auto sendErr = SendPBMessage(channel, requestID, nullptr, nullptr, messageError);
    if (!sendErr.IsNone() && err.IsNone()) {
        err = sendErr;
    }

    return AOS_ERROR_WRAP(err);
}

aos::Error IAMServer::ProcessCreateKey(Channel channel, uint64_t requestID, const aos::Array<uint8_t>& data)
{
    auto pbCreateKeyRequest  = aos::MakeUnique<iamanager_v4_CreateKeyRequest>(&mAllocator);
    auto pbCreateKeyResponse = aos::MakeUnique<iamanager_v4_CreateKeyResponse>(&mAllocator);

    auto err = DecodePBMessage(data, &iamanager_v4_CreateKeyRequest_msg, pbCreateKeyRequest.Get());

    strncpy(pbCreateKeyResponse->node_id, pbCreateKeyRequest->node_id, sizeof(pbCreateKeyResponse->node_id));
    strncpy(pbCreateKeyResponse->type, pbCreateKeyRequest->type, sizeof(pbCreateKeyResponse->type));

    if (err.IsNone()) {
        err = CheckPBNodeID(pbCreateKeyRequest->node_id);
    }

    if (err.IsNone()) {
        auto csr = aos::String(pbCreateKeyResponse->csr, sizeof(pbCreateKeyResponse->csr) - 1);

        err = mCertHandler->CreateKey(
            pbCreateKeyRequest->type, pbCreateKeyRequest->subject, pbCreateKeyRequest->password, csr);

        pbCreateKeyResponse->csr[csr.Size()] = 0;
    }

    auto sendErr
        = SendPBMessage(channel, requestID, &iamanager_v4_CreateKeyResponse_msg, pbCreateKeyResponse.Get(), err);
    if (!sendErr.IsNone() && err.IsNone()) {
        err = sendErr;
    }

    return AOS_ERROR_WRAP(err);
}

aos::Error IAMServer::ProcessApplyCert(Channel channel, uint64_t requestID, const aos::Array<uint8_t>& data)
{
    auto pbApplyCertRequest  = aos::MakeUnique<iamanager_v4_ApplyCertRequest>(&mAllocator);
    auto pbApplyCertResponse = aos::MakeUnique<iamanager_v4_ApplyCertResponse>(&mAllocator);

    auto err = DecodePBMessage(data, &iamanager_v4_ApplyCertRequest_msg, pbApplyCertRequest.Get());

    strncpy(pbApplyCertResponse->node_id, pbApplyCertRequest->node_id, sizeof(pbApplyCertResponse->node_id));
    strncpy(pbApplyCertResponse->type, pbApplyCertRequest->type, sizeof(pbApplyCertResponse->type));

    if (err.IsNone()) {
        err = CheckPBNodeID(pbApplyCertRequest->node_id);
    }

    if (err.IsNone()) {
        auto certInfo = aos::MakeUnique<aos::iam::certhandler::CertInfo>(&mAllocator);

        err = mCertHandler->ApplyCertificate(pbApplyCertRequest->type, pbApplyCertRequest->cert, *certInfo);
        if (err.IsNone()) {
            StringToPB(certInfo->mCertURL, pbApplyCertResponse->cert_url);
            err = aos::String(pbApplyCertResponse->serial, sizeof(pbApplyCertResponse->serial) - 1)
                      .ByteArrayToHex(certInfo->mSerial);
        }
    }

    auto sendErr
        = SendPBMessage(channel, requestID, &iamanager_v4_ApplyCertResponse_msg, pbApplyCertResponse.Get(), err);
    if (!sendErr.IsNone() && err.IsNone()) {
        err = sendErr;
    }

    return AOS_ERROR_WRAP(err);
}

aos::Error IAMServer::DecodePBMessage(const aos::Array<uint8_t>& data, const pb_msgdesc_t* fields, void* message)
{
    auto stream = pb_istream_from_buffer(data.Get(), data.Size());

    auto status = pb_decode(&stream, fields, message);
    if (!status) {
        LOG_ERR() << "Can't decode incoming message: " << PB_GET_ERROR(&stream);

        return AOS_ERROR_WRAP(aos::ErrorEnum::eRuntime);
    }

    return aos::ErrorEnum::eNone;
}
