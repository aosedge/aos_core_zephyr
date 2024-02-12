/*
 * Copyright (C) 2024 Renesas Electronics Corporation.
 * Copyright (C) 2024 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef IAMSERVER_HPP_
#define IAMSERVER_HPP_

#include <aos/iam/certhandler.hpp>

#include <proto/iamanager/v4/iamanager.pb.h>

#include "provisioning/provisioning.hpp"

#include "messagehandler.hpp"

/**
 * Number of IAM PB services.
 */
constexpr auto cIAMServicesCount = 2;

/**
 * IAM server instance.
 */
class IAMServer : public MessageHandler<aos::Max(iamanager_v4_NodesID_size, iamanager_v4_CertTypes_size,
                                            iamanager_v4_CreateKeyResponse_size, iamanager_v4_ApplyCertResponse_size),
                      aos::Max(iamanager_v4_SetOwnerRequest_size, iamanager_v4_ClearRequest_size,
                          iamanager_v4_EncryptDiskRequest_size, iamanager_v4_CreateKeyRequest_size,
                          iamanager_v4_ApplyCertRequest_size),
                      cIAMServicesCount> {
public:
    /*
     * IAM provisioning service.
     */
    static constexpr auto cServiceProvisioning      = "iamanager.v4.IAMProvisioningService";
    static constexpr auto cMethodGetAllNodeIDs      = "GetAllNodeIDs";
    static constexpr auto cMethodGetCertTypes       = "GetCertTypes";
    static constexpr auto cMethodSetOwner           = "SetOwner";
    static constexpr auto cMethodClear              = "Clear";
    static constexpr auto cMethodEncryptDisk        = "EncryptDisk";
    static constexpr auto cMethodFinishProvisioning = "FinishProvisioning";

    /*
     * IAM certificate service.
     */
    static constexpr auto cServiceCertificate = "iamanager.v4.IAMCertificateService";
    static constexpr auto cMethodCreateKey    = "CreateKey";
    static constexpr auto cMethodApplyCert    = "ApplyCert";

    /**
     * Creates IAM server.
     */
    IAMServer() = default;

    /**
     * Initializes CM client instance.
     *
     * @param certHandler certificate handler instance.
     * @param provisioning provisioning instance.
     * @param messageSender message sender instance.
     * @return aos::Error.
     */
    aos::Error Init(aos::iam::certhandler::CertHandlerItf& certHandler, ProvisioningItf& provisioning,
        MessageSenderItf& messageSender);

private:
    static constexpr auto cProvisioningServiceMethodCount = 6;
    static constexpr auto cCertificateServiceMethodCount  = 2;

    typedef aos::Error (IAMServer::*MethodCallback)(
        Channel channel, uint64_t requestID, const aos::Array<uint8_t>& data);

    struct MethodHandler {
        aos::StaticString<cMaxMethodLen> mMethodName;
        MethodCallback                   mCallback;
    };

    aos::Error ProcessMessage(Channel channel, PBServiceItf& service, const aos::String& methodName, uint64_t requestID,
        const aos::Array<uint8_t>& data) override;
    aos::Error ProcessGetAllNodeIDs(Channel channel, uint64_t requestID, const aos::Array<uint8_t>& data);
    aos::Error ProcessGetGetCertTypes(Channel channel, uint64_t requestID, const aos::Array<uint8_t>& data);
    aos::Error ProcessSetOwner(Channel channel, uint64_t requestID, const aos::Array<uint8_t>& data);
    aos::Error ProcessClear(Channel channel, uint64_t requestID, const aos::Array<uint8_t>& data);
    aos::Error ProcessEncryptDisk(Channel channel, uint64_t requestID, const aos::Array<uint8_t>& data);
    aos::Error ProcessFinishProvisioning(Channel channel, uint64_t requestID, const aos::Array<uint8_t>& data);
    aos::Error ProcessCreateKey(Channel channel, uint64_t requestID, const aos::Array<uint8_t>& data);
    aos::Error ProcessApplyCert(Channel channel, uint64_t requestID, const aos::Array<uint8_t>& data);
    aos::Error DecodePBMessage(const aos::Array<uint8_t>& data, const pb_msgdesc_t* fields, void* message);
    template <size_t cSize>
    constexpr aos::Error CheckPBNodeID(const char (&pbNodeID)[cSize])
    {
        if (strncmp(pbNodeID, cNodeID, cSize) != 0) {
            return aos::ErrorEnum::eFailed;
        }

        return aos::ErrorEnum::eNone;
    }

    aos::iam::certhandler::CertHandlerItf* mCertHandler {};
    ProvisioningItf*                       mProvisioning {};
    MessageSenderItf*                      mMessageSender {};

    PBService<cProvisioningServiceMethodCount> mProvisioningService;
    PBService<cCertificateServiceMethodCount>  mCertificateService;

    aos::StaticArray<MethodHandler, cProvisioningServiceMethodCount + cCertificateServiceMethodCount> mHandlers;

    aos::StaticAllocator<sizeof(aos::StaticString<cMaxServiceLen + cMaxMethodLen>)
        + aos::Max(sizeof(iamanager_v4_CreateKeyRequest) + sizeof(iamanager_v4_CreateKeyResponse),
            sizeof(iamanager_v4_ApplyCertRequest) + sizeof(iamanager_v4_ApplyCertResponse)
                + sizeof(aos::iam::certhandler::CertInfo))>
        mAllocator;
};

#endif
