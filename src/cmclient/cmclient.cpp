/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <unistd.h>

#include <pb_decode.h>
#include <tinycrypt/constants.h>
#include <tinycrypt/sha256.h>
#include <vchanapi.h>
#include <zephyr/kernel.h>

#include <aos/common/tools/memory.hpp>

#include "cmclient.hpp"
#include "log.hpp"

using namespace aos::sm::launcher;

/***********************************************************************************************************************
 * Public
 **********************************************************************************************************************/

CMClient::~CMClient()
{
    atomic_set_bit(&mFinishReadTrigger, 0);
    mThread.Join();
}

aos::Error CMClient::Init(LauncherItf& launcher, ResourceManager& resourceManager, DownloadReceiverItf& downloader)
{
    LOG_DBG() << "Initialize CM client";
    LOG_INF() << "Node ID: " << cNodeID << ", node type: " << cNodeType;

    mLauncher = &launcher;
    mResourceManager = &resourceManager;
    mDownloader = &downloader;

    auto err = mThread.Run([this](void*) {
        while (true) {
            ConnectToCM();

            auto err = ProcessMessages();
            if (!err.IsNone()) {
                LOG_ERR() << "Error processing messages: " << err;

                continue;
            }

            break;
        }
    });
    if (!err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    return aos::ErrorEnum::eNone;
}

aos::Error CMClient::InstancesRunStatus(const aos::Array<aos::InstanceStatus>& instances)
{
    aos::LockGuard lock(mMutex);

    mOutgoingMessage.which_SMOutgoingMessage = servicemanager_v3_SMOutgoingMessages_run_instances_status_tag;
    mOutgoingMessage.SMOutgoingMessage.run_instances_status
        = servicemanager_v3_RunInstancesStatus servicemanager_v3_RunInstancesStatus_init_zero;
    mOutgoingMessage.SMOutgoingMessage.run_instances_status.instances_count = instances.Size();

    for (size_t i = 0; i < instances.Size(); i++) {
        mOutgoingMessage.SMOutgoingMessage.run_instances_status.instances[i] = InstanceStatusToPB(instances[i]);
    }

    auto err = SendPbMessageToVchan();
    if (!err.IsNone()) {
        return err;
    }

    return aos::ErrorEnum::eNone;
}

aos::Error CMClient::InstancesUpdateStatus(const aos::Array<aos::InstanceStatus>& instances)
{
    aos::LockGuard lock(mMutex);

    mOutgoingMessage.which_SMOutgoingMessage = servicemanager_v3_SMOutgoingMessages_update_instances_status_tag;
    mOutgoingMessage.SMOutgoingMessage.update_instances_status
        = servicemanager_v3_UpdateInstancesStatus servicemanager_v3_UpdateInstancesStatus_init_zero;
    mOutgoingMessage.SMOutgoingMessage.update_instances_status.instances_count = instances.Size();

    for (size_t i = 0; i < instances.Size(); i++) {
        mOutgoingMessage.SMOutgoingMessage.update_instances_status.instances[i] = InstanceStatusToPB(instances[i]);
    }

    auto err = SendPbMessageToVchan();
    if (!err.IsNone()) {
        return err;
    }

    return aos::ErrorEnum::eNone;
}

aos::Error CMClient::SendImageContentRequest(const ImageContentRequest& request)
{
    aos::LockGuard lock(mMutex);

    mOutgoingMessage.which_SMOutgoingMessage = servicemanager_v3_SMOutgoingMessages_image_content_request_tag;
    mOutgoingMessage.SMOutgoingMessage.image_content_request = servicemanager_v3_ImageContentRequest_init_zero;

    LOG_DBG() << "Send image content request. Content type: " << request.contentType << " , URL: " << request.url;

    strncpy(mOutgoingMessage.SMOutgoingMessage.image_content_request.content_type,
        request.contentType.ToString().CStr(),
        sizeof(mOutgoingMessage.SMOutgoingMessage.image_content_request.content_type));
    strncpy(mOutgoingMessage.SMOutgoingMessage.image_content_request.url, request.url.CStr(),
        sizeof(mOutgoingMessage.SMOutgoingMessage.image_content_request.url));
    mOutgoingMessage.SMOutgoingMessage.image_content_request.request_id = request.requestID;

    auto err = SendPbMessageToVchan();
    if (!err.IsNone()) {
        return err;
    }

    return aos::ErrorEnum::eNone;
}

/***********************************************************************************************************************
 * Private
 **********************************************************************************************************************/

void CMClient::ConnectToCM()
{
    do {
        auto ret = vch_connect(cDomdID, cSmVchanPath, &mSMvchanHandler);
        if (ret != 0) {
            LOG_WRN() << "Can't connect to SM vchan error: " << aos::Error(ret);

            sleep(cConnectionTimeoutSec);

            continue;
        }

        break;
    } while (true);

    SendNodeConfiguration();

    auto err = mLauncher->RunLastInstances();
    if (!err.IsNone()) {
        LOG_ERR() << "Can't run last instances: " << err;
    }
}

aos::Error CMClient::ProcessMessages()
{
    VchanMessageHeader header;

    while (!atomic_test_bit(&mFinishReadTrigger, 0)) {
        auto err = ReadDataFromVChan(&mSMvchanHandler, &header, sizeof(VchanMessageHeader));
        if (!err.IsNone()) {
            return err;
        }

        err = ReadDataFromVChan(&mSMvchanHandler, mReceiveBuffer.Get(), header.dataSize);
        if (!err.IsNone()) {
            return err;
        }

        SHA256Digest calculatedDigest;

        err = CalculateSha256(mReceiveBuffer, header.dataSize, calculatedDigest);
        if (!err.IsNone()) {
            LOG_ERR() << "Can't calculate SHA256: " << err;
            continue;
        }

        if (0 != memcmp(calculatedDigest, header.sha256, sizeof(SHA256Digest))) {
            LOG_ERR() << "SHA256 mismatch";
            continue;
        }

        mIncomingMessage = servicemanager_v3_SMIncomingMessages_init_zero;

        auto stream = pb_istream_from_buffer((const uint8_t*)mReceiveBuffer.Get(), header.dataSize);

        auto status = pb_decode(&stream, servicemanager_v3_SMIncomingMessages_fields, &mIncomingMessage);
        if (!status) {
            LOG_ERR() << "Can't decode incoming PB message: " << PB_GET_ERROR(&stream);
            continue;
        }

        switch (mIncomingMessage.which_SMIncomingMessage) {
        case servicemanager_v3_SMIncomingMessages_get_unit_config_status_tag:
            ProcessGetUnitConfigStatus();
            break;

        case servicemanager_v3_SMIncomingMessages_check_unit_config_tag:
            ProcessCheckUnitConfig();
            break;

        case servicemanager_v3_SMIncomingMessages_set_unit_config_tag:
            ProcessSetUnitConfig();
            break;

        case servicemanager_v3_SMIncomingMessages_run_instances_tag:
            ProcessRunInstancesMessage();
            break;

        case servicemanager_v3_SMIncomingMessages_system_log_request_tag:
            break;

        case servicemanager_v3_SMIncomingMessages_instance_log_request_tag:
            break;

        case servicemanager_v3_SMIncomingMessages_instance_crash_log_request_tag:
            break;

        case servicemanager_v3_SMIncomingMessages_image_content_info_tag:
            ProcessImageContentInfo();
            break;

        case servicemanager_v3_SMIncomingMessages_image_content_tag:
            ProcessImageContentChunk();
            break;

        default:
            LOG_WRN() << "Receive unsupported message tag: " << mIncomingMessage.which_SMIncomingMessage;
            break;
        }
    }

    return aos::ErrorEnum::eNone;
}

void CMClient::SendNodeConfiguration()
{
    aos::LockGuard lock(mMutex);

    mOutgoingMessage.which_SMOutgoingMessage = servicemanager_v3_SMOutgoingMessages_node_configuration_tag;
    mOutgoingMessage.SMOutgoingMessage.node_configuration
        = servicemanager_v3_NodeConfiguration servicemanager_v3_NodeConfiguration_init_zero;
    auto&                                     config = mOutgoingMessage.SMOutgoingMessage.node_configuration;

    strncpy(config.node_id, cNodeID, sizeof(config.node_id));
    strncpy(config.node_type, cNodeType, sizeof(config.node_type));
    config.remote_node = true;
    config.runner_features_count = 1;
    strncpy(config.runner_features[0], "xrun", sizeof(config.runner_features[0]));
    config.num_cpus = cNumCPUs;
    config.total_ram = cTotalRAM;
    config.partitions_count = 1;
    strncpy(config.partitions[0].name, "aos", sizeof(config.partitions[0].name));
    config.partitions[0].total_size = cPartitionSize;
    config.partitions[0].types_count = 1;
    strncpy(config.partitions[0].types[0], "services", sizeof(config.partitions[0].types[0]));

    auto err = SendPbMessageToVchan();
    if (!err.IsNone()) {
        LOG_ERR() << "Can't send node configuration: " << err;
    }
}

aos::Error CMClient::SendPbMessageToVchan()
{
    auto outStream
        = pb_ostream_from_buffer(static_cast<pb_byte_t*>(mSendBuffer.Get()), servicemanager_v3_SMOutgoingMessages_size);

    auto status = pb_encode(&outStream, servicemanager_v3_SMOutgoingMessages_fields, &mOutgoingMessage);
    if (!status) {
        LOG_ERR() << "Encoding failed: " << PB_GET_ERROR(&outStream);

        return AOS_ERROR_WRAP(aos::ErrorEnum::eRuntime);
    }

    VchanMessageHeader header = {static_cast<uint32_t>(outStream.bytes_written)};

    auto err = CalculateSha256(mSendBuffer, header.dataSize, header.sha256);
    if (!err.IsNone()) {
        return err;
    }

    err = SendBufferToVchan(&mSMvchanHandler, (uint8_t*)&header, sizeof(VchanMessageHeader));
    if (!err.IsNone()) {
        return err;
    }

    return SendBufferToVchan(&mSMvchanHandler, static_cast<uint8_t*>(mSendBuffer.Get()), header.dataSize);
}

aos::Error CMClient::SendBufferToVchan(vch_handle* vChanHandler, const uint8_t* buffer, size_t msgSize)
{
    for (size_t offset = 0; offset < msgSize;) {
        auto ret = vch_write(vChanHandler, buffer + offset, msgSize - offset);
        if (ret < 0) {
            return AOS_ERROR_WRAP(ret);
        }

        offset += ret;
    }

    return aos::ErrorEnum::eNone;
}

aos::Error CMClient::CalculateSha256(const aos::Buffer& buffer, size_t msgSize, SHA256Digest& digest)
{
    tc_sha256_state_struct s;

    auto ret = tc_sha256_init(&s);
    if (TC_CRYPTO_SUCCESS != ret) {
        return AOS_ERROR_WRAP(ret);
    }

    ret = tc_sha256_update(&s, static_cast<uint8_t*>(buffer.Get()), msgSize);
    if (TC_CRYPTO_SUCCESS != ret) {
        return AOS_ERROR_WRAP(ret);
    }

    ret = tc_sha256_final(digest, &s);
    if (TC_CRYPTO_SUCCESS != ret) {
        return AOS_ERROR_WRAP(ret);
    }

    return aos::ErrorEnum::eNone;
}

servicemanager_v3_InstanceStatus CMClient::InstanceStatusToPB(const aos::InstanceStatus& instanceStatus) const
{
    servicemanager_v3_InstanceStatus pbStatus = servicemanager_v3_InstanceStatus_init_zero;

    pbStatus.aos_version = instanceStatus.mAosVersion;
    pbStatus.has_instance = true;
    pbStatus.instance.instance = instanceStatus.mInstanceIdent.mInstance;
    strncpy(pbStatus.instance.service_id, instanceStatus.mInstanceIdent.mServiceID.CStr(),
        sizeof(pbStatus.instance.service_id));
    strncpy(pbStatus.instance.subject_id, instanceStatus.mInstanceIdent.mSubjectID.CStr(),
        sizeof(pbStatus.instance.subject_id));
    strncpy(pbStatus.run_state, instanceStatus.mRunState.ToString().CStr(), sizeof(pbStatus.run_state));

    if (!instanceStatus.mError.IsNone()) {
        pbStatus.has_error_info = true;
        strncpy(pbStatus.error_info.message, instanceStatus.mError.Message(), sizeof(pbStatus.error_info.message));
    }

    return pbStatus;
}

void CMClient::ProcessGetUnitConfigStatus()
{
    aos::LockGuard lock(mMutex);

    mOutgoingMessage.which_SMOutgoingMessage = servicemanager_v3_SMOutgoingMessages_unit_config_status_tag;
    mOutgoingMessage.SMOutgoingMessage.unit_config_status = servicemanager_v3_UnitConfigStatus_init_zero;

    auto err
        = mResourceManager->GetUnitConfigInfo(mOutgoingMessage.SMOutgoingMessage.unit_config_status.vendor_version);
    if (!err.IsNone()) {
        strncpy(mOutgoingMessage.SMOutgoingMessage.unit_config_status.error, err.Message(),
            sizeof(mOutgoingMessage.SMOutgoingMessage.unit_config_status.error));
    }

    if (!(err = SendPbMessageToVchan()).IsNone()) {
        LOG_ERR() << "Can't send unit configuration status: " << err;
    }
}

void CMClient::ProcessCheckUnitConfig()
{
    auto err = mResourceManager->CheckUnitConfig(mIncomingMessage.SMIncomingMessage.check_unit_config.vendor_version,
        mIncomingMessage.SMIncomingMessage.check_unit_config.unit_config);

    aos::LockGuard lock(mMutex);

    mOutgoingMessage.which_SMOutgoingMessage = servicemanager_v3_SMOutgoingMessages_unit_config_status_tag;
    mOutgoingMessage.SMOutgoingMessage.unit_config_status = servicemanager_v3_UnitConfigStatus_init_zero;

    strncpy(mOutgoingMessage.SMOutgoingMessage.unit_config_status.vendor_version,
        mIncomingMessage.SMIncomingMessage.check_unit_config.vendor_version,
        sizeof(mOutgoingMessage.SMOutgoingMessage.unit_config_status.vendor_version));
    if (!err.IsNone()) {
        strncpy(mOutgoingMessage.SMOutgoingMessage.unit_config_status.error, err.Message(),
            sizeof(mOutgoingMessage.SMOutgoingMessage.unit_config_status.error));
    }

    if (!(err = SendPbMessageToVchan()).IsNone()) {
        LOG_ERR() << "Can't send unit configuration status: " << err;
    }
}

void CMClient::ProcessSetUnitConfig()
{
    auto err = mResourceManager->UpdateUnitConfig(mIncomingMessage.SMIncomingMessage.set_unit_config.vendor_version,
        mIncomingMessage.SMIncomingMessage.set_unit_config.unit_config);

    aos::LockGuard lock(mMutex);

    mOutgoingMessage.which_SMOutgoingMessage = servicemanager_v3_SMOutgoingMessages_unit_config_status_tag;
    mOutgoingMessage.SMOutgoingMessage.unit_config_status = servicemanager_v3_UnitConfigStatus_init_zero;

    strncpy(mOutgoingMessage.SMOutgoingMessage.unit_config_status.vendor_version,
        mIncomingMessage.SMIncomingMessage.set_unit_config.vendor_version,
        sizeof(mOutgoingMessage.SMOutgoingMessage.unit_config_status.vendor_version));
    if (!err.IsNone()) {
        strncpy(mOutgoingMessage.SMOutgoingMessage.unit_config_status.error, err.Message(),
            sizeof(mOutgoingMessage.SMOutgoingMessage.unit_config_status.error));
    }

    if (!(err = SendPbMessageToVchan()).IsNone()) {
        LOG_ERR() << "Can't send unit configuration status: " << err;
    }
}

void CMClient::ProcessRunInstancesMessage()
{
    LOG_DBG() << "Process run instances";

    aos::BufferAllocator<> allocator(mReceiveBuffer);

    auto runInstances = aos::MakeUnique<aos::InstanceInfoStaticArray>(&allocator);

    runInstances->Resize(mIncomingMessage.SMIncomingMessage.run_instances.instances_count);

    for (auto i = 0; i < mIncomingMessage.SMIncomingMessage.run_instances.instances_count; i++) {
        const auto& pbInstance = mIncomingMessage.SMIncomingMessage.run_instances.instances[i];
        auto&       instance = (*runInstances)[i];

        if (pbInstance.has_instance) {
            instance.mInstanceIdent.mServiceID = pbInstance.instance.service_id;
            instance.mInstanceIdent.mSubjectID = pbInstance.instance.subject_id;
            instance.mInstanceIdent.mInstance = pbInstance.instance.instance;
        }

        instance.mPriority = pbInstance.priority;
        instance.mStatePath = pbInstance.state_path;
        instance.mStoragePath = pbInstance.storage_path;
        instance.mUID = pbInstance.uid;
    }

    auto desiredServices = aos::MakeUnique<aos::ServiceInfoStaticArray>(&allocator);

    desiredServices->Resize(mIncomingMessage.SMIncomingMessage.run_instances.services_count);

    for (auto i = 0; i < mIncomingMessage.SMIncomingMessage.run_instances.services_count; i++) {
        const auto& pbService = mIncomingMessage.SMIncomingMessage.run_instances.services[0];
        auto&       service = (*desiredServices)[i];

        if (pbService.has_version_info) {
            service.mVersionInfo.mAosVersion = pbService.version_info.aos_version;
            service.mVersionInfo.mVendorVersion = pbService.version_info.vendor_version;
            service.mVersionInfo.mDescription = pbService.version_info.description;
        }

        service.mServiceID = pbService.service_id;
        service.mProviderID = pbService.provider_id;
        service.mGID = pbService.gid;
        service.mURL = pbService.url;
        memcpy(service.mSHA256, pbService.sha256.bytes, aos::cSHA256Size);
        memcpy(service.mSHA512, pbService.sha512.bytes, aos::cSHA512Size);
        service.mSize = pbService.size;
    }

    auto desiredLayers = aos::MakeUnique<aos::LayerInfoStaticArray>(&allocator);

    desiredLayers->Resize(mIncomingMessage.SMIncomingMessage.run_instances.layers_count);

    for (auto i = 0; i < mIncomingMessage.SMIncomingMessage.run_instances.layers_count; i++) {
        const auto& pbLayer = mIncomingMessage.SMIncomingMessage.run_instances.layers[i];
        auto&       layer = (*desiredLayers)[i];

        if (pbLayer.has_version_info) {
            layer.mVersionInfo.mAosVersion = pbLayer.version_info.aos_version;
            layer.mVersionInfo.mVendorVersion = aos::String(pbLayer.version_info.vendor_version);
            layer.mVersionInfo.mDescription = aos::String(pbLayer.version_info.description);
        }

        layer.mLayerID = aos::String(pbLayer.layer_id);
        layer.mLayerDigest = aos::String(pbLayer.digest);
        layer.mURL = aos::String(pbLayer.url);
        memcpy(layer.mSHA256, pbLayer.sha256.bytes, aos::cSHA256Size);
        memcpy(layer.mSHA512, pbLayer.sha512.bytes, aos::cSHA512Size);
        layer.mSize = pbLayer.size;
    }

    auto err = mLauncher->RunInstances(*desiredServices, *desiredLayers, *runInstances,
        mIncomingMessage.SMIncomingMessage.run_instances.force_restart);
    if (!err.IsNone()) {
        LOG_ERR() << "Can't run instances: " << err;
    }
}

void CMClient::ProcessImageContentInfo()
{
    LOG_DBG() << "Process image content info";

    aos::BufferAllocator<> allocator(mReceiveBuffer);

    auto contentInfo = aos::MakeUnique<ImageContentInfo>(&allocator);

    if (strlen(mIncomingMessage.SMIncomingMessage.image_content_info.error) != 0) {
        contentInfo->error = mIncomingMessage.SMIncomingMessage.image_content_info.error;
    } else {
        contentInfo->requestID = mIncomingMessage.SMIncomingMessage.image_content_info.request_id;
        for (auto i = 0; i < mIncomingMessage.SMIncomingMessage.image_content_info.image_files_count; i++) {
            servicemanager_v3_ImageFile& pbImageFile
                = mIncomingMessage.SMIncomingMessage.image_content_info.image_files[i];
            FileInfo fileInfo;

            fileInfo.size = pbImageFile.size;
            fileInfo.relativePath = pbImageFile.relative_path;
            memcpy(fileInfo.sha256.Get(), pbImageFile.sha256.bytes, aos::cSHA256Size);

            contentInfo->files.PushBack(fileInfo);
        }
    }

    auto err = mDownloader->ReceiveImageContentInfo(*contentInfo);
    if (!err.IsNone()) {
        LOG_ERR() << "Can't receive image content info: " << err;
    }
}

void CMClient::ProcessImageContentChunk()
{

    aos::BufferAllocator<> allocator(mReceiveBuffer);

    auto chunk = aos::MakeUnique<FileChunk>(&allocator);

    chunk->part = mIncomingMessage.SMIncomingMessage.image_content.part;
    chunk->partsCount = mIncomingMessage.SMIncomingMessage.image_content.parts_count;
    chunk->relativePath = mIncomingMessage.SMIncomingMessage.image_content.relative_path;
    chunk->requestID = mIncomingMessage.SMIncomingMessage.image_content.request_id;
    chunk->data.Resize(mIncomingMessage.SMIncomingMessage.image_content.data.size);
    memcpy(chunk->data.Get(), mIncomingMessage.SMIncomingMessage.image_content.data.bytes,
        mIncomingMessage.SMIncomingMessage.image_content.data.size);

    auto err = mDownloader->ReceiveFileChunk(*chunk);
    if (!err.IsNone()) {
        LOG_ERR() << "Can't receive file chunk: " << err;
    }
}

aos::Error CMClient::ReadDataFromVChan(vch_handle* vchanHandler, void* des, size_t size)
{
    size_t readSize = 0;

    while (readSize < size && !atomic_test_bit(&mFinishReadTrigger, 0)) {
        aos::LockGuard lock(mMutex);

        auto read = vch_read(vchanHandler, reinterpret_cast<uint8_t*>(des) + readSize, size - readSize);
        if (read < 0) {
            return AOS_ERROR_WRAP(read);
        }

        if (read == 0) {
            k_yield();
            continue;
        }

        readSize += read;
    }
}
