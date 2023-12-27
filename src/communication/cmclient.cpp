/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <aos/common/tools/memory.hpp>

#include <pb_decode.h>
#include <pb_encode.h>

#include "utils/pbconvert.hpp"

#include "cmclient.hpp"
#include "log.hpp"

/***********************************************************************************************************************
 * Public
 **********************************************************************************************************************/

aos::Error CMClient::Init(
    aos::sm::launcher::LauncherItf& launcher, ResourceManagerItf& resourceManager, MessageSenderItf& messageSender)
{
    LOG_DBG() << "Initialize CM client";

    mLauncher        = &launcher;
    mResourceManager = &resourceManager;
    mMessageSender   = &messageSender;

    return aos::ErrorEnum::eNone;
}

aos::Error CMClient::ProcessMessage(const aos::String& methodName, uint64_t requestID, const aos::Array<uint8_t>& data)
{
    (void)methodName;

    auto message = aos::MakeUnique<servicemanager_v3_SMIncomingMessages>(&mAllocator);
    auto stream  = pb_istream_from_buffer(data.Get(), data.Size());

    auto status = pb_decode(&stream, servicemanager_v3_SMIncomingMessages_fields, message.Get());
    if (!status) {
        LOG_ERR() << "Can't decode incoming message: " << PB_GET_ERROR(&stream);
        return AOS_ERROR_WRAP(aos::ErrorEnum::eRuntime);
    }

    aos::Error err;

    switch (message->which_SMIncomingMessage) {
    case servicemanager_v3_SMIncomingMessages_get_unit_config_status_tag:
        err = ProcessGetUnitConfigStatus();
        break;

    case servicemanager_v3_SMIncomingMessages_check_unit_config_tag:
        err = ProcessCheckUnitConfig(message->SMIncomingMessage.check_unit_config);
        break;

    case servicemanager_v3_SMIncomingMessages_set_unit_config_tag:
        err = ProcessSetUnitConfig(message->SMIncomingMessage.set_unit_config);
        break;

    case servicemanager_v3_SMIncomingMessages_run_instances_tag:
        err = ProcessRunInstances(message->SMIncomingMessage.run_instances);
        break;

    case servicemanager_v3_SMIncomingMessages_image_content_info_tag:
        break;

    case servicemanager_v3_SMIncomingMessages_image_content_tag:
        break;

    default:
        LOG_WRN() << "Receive unsupported message tag: " << message->which_SMIncomingMessage;
        break;
    }

    return err;
}

/***********************************************************************************************************************
 * Private
 **********************************************************************************************************************/

aos::Error CMClient::ProcessGetUnitConfigStatus()
{
    LOG_DBG() << "Receive SM message: message = GetUnitConfigStatus";

    auto  outgoingMessage = aos::MakeUnique<servicemanager_v3_SMOutgoingMessages>(&mAllocator);
    auto& pbConfigStatus  = outgoingMessage->SMOutgoingMessage.unit_config_status;

    outgoingMessage->which_SMOutgoingMessage = servicemanager_v3_SMOutgoingMessages_unit_config_status_tag;
    pbConfigStatus                           = servicemanager_v3_UnitConfigStatus_init_zero;

    aos::StaticString<aos::cVendorVersionLen> version;

    auto err = mResourceManager->GetUnitConfigInfo(version);
    if (!err.IsNone()) {
        ErrorToPB(err, pbConfigStatus.error);
    }

    StringToPB(version, pbConfigStatus.vendor_version);

    LOG_DBG() << "Send SM message: message = UnitConfigStatus";

    return SendOutgoingMessage(*outgoingMessage);
}

aos::Error CMClient::ProcessCheckUnitConfig(const servicemanager_v3_CheckUnitConfig& pbUnitConfig)
{
    LOG_DBG() << "Receive SM message: message = CheckUnitConfig";

    auto  outgoingMessage = aos::MakeUnique<servicemanager_v3_SMOutgoingMessages>(&mAllocator);
    auto& pbConfigStatus  = outgoingMessage->SMOutgoingMessage.unit_config_status;

    outgoingMessage->which_SMOutgoingMessage = servicemanager_v3_SMOutgoingMessages_unit_config_status_tag;
    pbConfigStatus                           = servicemanager_v3_UnitConfigStatus_init_zero;

    auto err = mResourceManager->CheckUnitConfig(pbUnitConfig.vendor_version, pbUnitConfig.unit_config);
    if (!err.IsNone()) {
        ErrorToPB(err, pbConfigStatus.error);
    }

    aos::StaticString<aos::cVendorVersionLen> version;

    PBToString(pbUnitConfig.vendor_version, version);
    StringToPB(version, pbConfigStatus.vendor_version);

    LOG_DBG() << "Send SM message: message = UnitConfigStatus";

    return SendOutgoingMessage(*outgoingMessage);
}

aos::Error CMClient::ProcessSetUnitConfig(const servicemanager_v3_SetUnitConfig& pbUnitConfig)
{
    LOG_DBG() << "Receive SM message: message = SetUnitConfig";

    auto  outgoingMessage = aos::MakeUnique<servicemanager_v3_SMOutgoingMessages>(&mAllocator);
    auto& pbConfigStatus  = outgoingMessage->SMOutgoingMessage.unit_config_status;

    outgoingMessage->which_SMOutgoingMessage = servicemanager_v3_SMOutgoingMessages_unit_config_status_tag;
    pbConfigStatus                           = servicemanager_v3_UnitConfigStatus_init_zero;

    auto err = mResourceManager->UpdateUnitConfig(pbUnitConfig.vendor_version, pbUnitConfig.unit_config);
    if (!err.IsNone()) {
        ErrorToPB(err, pbConfigStatus.error);
    }

    strncpy(pbConfigStatus.vendor_version, pbUnitConfig.vendor_version, sizeof(pbConfigStatus.vendor_version));

    LOG_DBG() << "Send SM message: message = UnitConfigStatus";

    return SendOutgoingMessage(*outgoingMessage);
}

aos::Error CMClient::ProcessRunInstances(const servicemanager_v3_RunInstances& pbRunInstances)
{
    LOG_DBG() << "Receive SM message: message = RunInstances";

    auto aosServices = aos::MakeUnique<aos::ServiceInfoStaticArray>(&mAllocator);

    aosServices->Resize(pbRunInstances.services_count);

    for (auto i = 0; i < pbRunInstances.services_count; i++) {
        const auto& pbService  = pbRunInstances.services[i];
        auto&       aosService = (*aosServices)[i];

        if (pbService.has_version_info) {
            PBToVersionInfo(pbService.version_info, aosService.mVersionInfo);
        }

        PBToString(pbService.service_id, aosService.mServiceID);
        PBToString(pbService.provider_id, aosService.mProviderID);
        aosService.mGID = pbService.gid;
        aosService.mURL = pbService.url;
        PBToByteArray(pbService.sha256, aosService.mSHA256);
        PBToByteArray(pbService.sha512, aosService.mSHA512);
        aosService.mSize = pbService.size;
    }

    auto aosLayers = aos::MakeUnique<aos::LayerInfoStaticArray>(&mAllocator);

    aosLayers->Resize(pbRunInstances.layers_count);

    for (auto i = 0; i < pbRunInstances.layers_count; i++) {
        const auto& pbLayer  = pbRunInstances.layers[i];
        auto&       aosLayer = (*aosLayers)[i];

        if (pbLayer.has_version_info) {
            PBToVersionInfo(pbLayer.version_info, aosLayer.mVersionInfo);
        }

        PBToString(pbLayer.layer_id, aosLayer.mLayerID);
        PBToString(pbLayer.digest, aosLayer.mLayerDigest);
        PBToString(pbLayer.url, aosLayer.mURL);
        PBToByteArray(pbLayer.sha256, aosLayer.mSHA256);
        PBToByteArray(pbLayer.sha512, aosLayer.mSHA512);
        aosLayer.mSize = pbLayer.size;
    }

    auto aosInstances = aos::MakeUnique<aos::InstanceInfoStaticArray>(&mAllocator);

    aosInstances->Resize(pbRunInstances.instances_count);

    for (auto i = 0; i < pbRunInstances.instances_count; i++) {
        const auto& pbInstance  = pbRunInstances.instances[i];
        auto&       aosInstance = (*aosInstances)[i];

        if (pbInstance.has_instance) {
            PBToString(pbInstance.instance.service_id, aosInstance.mInstanceIdent.mServiceID);
            PBToString(pbInstance.instance.subject_id, aosInstance.mInstanceIdent.mSubjectID);
            aosInstance.mInstanceIdent.mInstance = pbInstance.instance.instance;
        }

        aosInstance.mUID      = pbInstance.uid;
        aosInstance.mPriority = pbInstance.priority;
        PBToString(pbInstance.storage_path, aosInstance.mStoragePath);
        PBToString(pbInstance.state_path, aosInstance.mStatePath);
    }

    auto err = mLauncher->RunInstances(*aosServices, *aosLayers, *aosInstances, pbRunInstances.force_restart);
    if (!err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    return aos::ErrorEnum::eNone;
}

aos::Error CMClient::SendOutgoingMessage(const servicemanager_v3_SMOutgoingMessages& message, aos::Error messageError)
{
    auto outStream = pb_ostream_from_buffer(static_cast<pb_byte_t*>(mSendBuffer.Get()), mSendBuffer.Size());

    auto status = pb_encode(&outStream, servicemanager_v3_SMOutgoingMessages_fields, &message);
    if (!status) {
        LOG_ERR() << "Can't encode outgoing message: " << PB_GET_ERROR(&outStream);

        if (messageError.IsNone()) {
            messageError = AOS_ERROR_WRAP(aos::ErrorEnum::eRuntime);
        }
    }

    return mMessageSender->SendMessage(ChannelEnum::eSecure, AOS_VCHAN_SM, "", 0,
        aos::Array<uint8_t>(static_cast<uint8_t*>(mSendBuffer.Get()), outStream.bytes_written), messageError);
}
