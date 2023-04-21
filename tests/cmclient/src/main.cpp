/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/tc_util.h>
#include <zephyr/ztest.h>

#include <pb_decode.h>
#include <servicemanager.pb.h>
#include <tinycrypt/constants.h>
#include <tinycrypt/sha256.h>
#include <vchanapi.h>

#include <aos/common/tools/log.hpp>

#include "cmclient/cmclient.hpp"
#include "resourcemanager/resourcemanager.hpp"

#include "mockdownloader.hpp"
#include "mocklauncher.hpp"

static constexpr char testVersion[6] = "1.0.1";

bool                                 gWaitHeader = true;
bool                                 gReadHead = true;
bool                                 gShutdown = false;
bool                                 gReadyToRead = true;
bool                                 gReadyToCheck = false;
VChanMessageHeader                   gCurrentHeader;
servicemanager_v3_SMOutgoingMessages gReceivedMessage;
servicemanager_v3_SMIncomingMessages gSendMessage;
uint8_t                              gSendBuffer[servicemanager_v3_SMIncomingMessages_size];
aos::Mutex                           gWaitMessageMutex;
aos::ConditionalVariable             gWaitMessageCondVar(gWaitMessageMutex);
aos::Mutex                           gReadMutex;
aos::ConditionalVariable             gReadCondVar(gReadMutex);
size_t                               gCurrentSendBufferSize;
CMClient                             gTestCMClient;
MockLauncher                         gMockLauncher(gWaitMessageCondVar, gWaitMessageMutex, gReadyToCheck);
MockDownloader                       gMockDownloader(gWaitMessageCondVar, gWaitMessageMutex, gReadyToCheck);

using namespace aos;

void TestLogCallback(LogModule module, LogLevel level, const aos::String& message)
{
    printk("[client] %s \n", message.CStr());
}

aos::Error ResourceManager::GetUnitConfigInfo(char* version) const
{
    printk("[test] GetUnitConfigInfo\n");

    strcpy(version, testVersion);

    return aos::ErrorEnum::eNone;
}

aos::Error ResourceManager::CheckUnitConfig(const char* version, const char* unitConfig) const
{
    return aos::ErrorEnum::eNone;
}

aos::Error ResourceManager::UpdateUnitConfig(const char* version, const char* unitConfig)
{
    return aos::ErrorEnum::eNone;
}

int vch_open(domid_t domain, const char* path, size_t min_rs, size_t min_ws, struct vch_handle* h)
{
    return 0;
}

int vch_connect(domid_t domain, const char* path, struct vch_handle* h)
{
    printk("[test] Connection Done\n");
    {
        UniqueLock lock(gWaitMessageMutex);
        gReadyToCheck = true;
        gWaitMessageCondVar.NotifyOne();
    }

    {
        UniqueLock lock(gReadMutex);
        gReadCondVar.Wait([&] { return gReadyToRead; });
        gReadyToRead = false;
    }

    return 0;
}

void vch_close(struct vch_handle* h)
{
}

int vch_read(struct vch_handle* h, void* buf, size_t size)
{
    printk("[test] Start read  %d \n", size);
    if (gReadHead) {
        UniqueLock lock(gReadMutex);
        gReadCondVar.Wait([&] { return gReadyToRead; });
        gReadyToRead = false;
        if (gShutdown) {
            return -1;
        }
    }

    if (gReadHead) {
        auto outStream = pb_ostream_from_buffer(gSendBuffer, servicemanager_v3_SMIncomingMessages_size);
        auto status = pb_encode(&outStream, servicemanager_v3_SMIncomingMessages_fields, &gSendMessage);
        zassert_true(status, "Encoding failed: %s\n", PB_GET_ERROR(&outStream));

        VChanMessageHeader     header;
        tc_sha256_state_struct s;

        int err = tc_sha256_init(&s);
        zassert_false((TC_CRYPTO_SUCCESS != err), "Can't init SHA256: %d", err);

        err = tc_sha256_update(&s, gSendBuffer, outStream.bytes_written);
        zassert_false((TC_CRYPTO_SUCCESS != err), "Can't hash message: %d", err);

        err = tc_sha256_final(header.sha256, &s);
        zassert_false((TC_CRYPTO_SUCCESS != err), "Can't finish hash message: %d", err);

        header.dataSize = outStream.bytes_written;

        memcpy(buf, static_cast<void*>(&header), sizeof(VChanMessageHeader));
        gCurrentSendBufferSize = outStream.bytes_written;
    } else {
        memcpy(buf, gSendBuffer, gCurrentSendBufferSize);
    }

    gReadHead = !gReadHead;

    printk("[test] END read  %d \n", size);
    return size;
}

int vch_write(struct vch_handle* h, const void* buf, size_t size)
{
    printk("[test] Start write %d header %d \n", size, gWaitHeader);

    if (gWaitHeader) {
        gWaitHeader = false;
        zassert_equal(size, sizeof(VChanMessageHeader), "Incorrect header size");
        memcpy(&gCurrentHeader, buf, sizeof(VChanMessageHeader));

        return size;
    }

    gWaitHeader = true;

    zassert_equal(size, gCurrentHeader.dataSize, "Header and message length mismatch");

    tc_sha256_state_struct s;
    uint8_t                sha256[32];

    int ret = tc_sha256_init(&s);
    zassert_false((TC_CRYPTO_SUCCESS != ret), "Can't init SHA256: %d", ret);

    ret = tc_sha256_update(&s, (const uint8_t*)buf, size);
    zassert_false((TC_CRYPTO_SUCCESS != ret), "Can't hash message: %d", ret);

    ret = tc_sha256_final(sha256, &s);
    zassert_false((TC_CRYPTO_SUCCESS != ret), "Can't finish hash message: %d", ret);
    zassert_false((0 != memcmp(sha256, gCurrentHeader.sha256, 32)), "Sha256 mismatch");

    gReceivedMessage = servicemanager_v3_SMOutgoingMessages servicemanager_v3_SMOutgoingMessages_init_zero;

    pb_istream_t stream = pb_istream_from_buffer((const pb_byte_t*)buf, size);
    auto         status = pb_decode(&stream, servicemanager_v3_SMOutgoingMessages_fields, &gReceivedMessage);
    zassert_true(status, "Decoding failed: %s", PB_GET_ERROR(&stream));

    printk("[test] Notify msg %d \n", gReceivedMessage.which_SMOutgoingMessage);
    {
        UniqueLock lock(gWaitMessageMutex);
        gReadyToCheck = true;
        gWaitMessageCondVar.NotifyOne();
    }

    return size;
}

void tesUpdateAndRunInstanceStatuses()
{
    StaticArray<InstanceStatus, 1> instances;
    InstanceStatus                 status;

    status.mInstanceIdent.mServiceID = aos::String("service1");
    status.mInstanceIdent.mSubjectID = aos::String("subject1");
    status.mInstanceIdent.mInstance = 0;
    status.mAosVersion = 12;
    status.mRunState = InstanceRunStateEnum::eActive;
    status.mError = ErrorEnum::eNone;

    instances.PushBack(status);

    auto err = gTestCMClient.InstancesRunStatus(instances);
    zassert_true(err.IsNone(), "Error send run status: %s", err.Message());
    zassert_equal(gReceivedMessage.which_SMOutgoingMessage,
        servicemanager_v3_SMOutgoingMessages_run_instances_status_tag, "Instance run status expected %d",
        gReceivedMessage.which_SMOutgoingMessage);
    zassert_equal(
        gReceivedMessage.SMOutgoingMessage.run_instances_status.instances_count, 1, "Incorrect run instances count");
    zassert_false(
        strcmp(gReceivedMessage.SMOutgoingMessage.run_instances_status.instances[0].instance.service_id, "service1"),
        "Incorrect Service id  %s",
        gReceivedMessage.SMOutgoingMessage.run_instances_status.instances[0].instance.service_id);

    err = gTestCMClient.InstancesUpdateStatus(instances);
    zassert_true(err.IsNone(), "Error send update status: %s", err.Message());
    zassert_equal(gReceivedMessage.which_SMOutgoingMessage,
        servicemanager_v3_SMOutgoingMessages_update_instances_status_tag, "Instance run status expected %d",
        gReceivedMessage.which_SMOutgoingMessage);
    zassert_equal(gReceivedMessage.SMOutgoingMessage.update_instances_status.instances_count, 1,
        "Incorrect update instances count");
    zassert_false(
        strcmp(gReceivedMessage.SMOutgoingMessage.update_instances_status.instances[0].instance.subject_id, "subject1"),
        "Incorrect Subject id  %s",
        gReceivedMessage.SMOutgoingMessage.update_instances_status.instances[0].instance.subject_id);
}

void testGetUnitConfigStatus()
{
    gReadyToCheck = false; // skip previous notify
    gSendMessage.which_SMIncomingMessage = servicemanager_v3_SMIncomingMessages_get_unit_config_status_tag;

    {
        UniqueLock lock(gReadMutex);
        gReadyToRead = true;
        gReadCondVar.NotifyOne();
    }

    // Wait node config status
    {
        UniqueLock lock(gWaitMessageMutex);
        gWaitMessageCondVar.Wait([&] { return gReadyToCheck; });
        gReadyToCheck = false;
    }

    zassert_equal(gReceivedMessage.which_SMOutgoingMessage, servicemanager_v3_SMOutgoingMessages_unit_config_status_tag,
        "Unit configuration status expected %d", gReceivedMessage.which_SMOutgoingMessage);
    zassert_false((0 != strcmp(gReceivedMessage.SMOutgoingMessage.unit_config_status.vendor_version, testVersion)),
        "Incorrect unit config version");
}

void testCheckUnitConfig()
{
    gSendMessage.which_SMIncomingMessage = servicemanager_v3_SMIncomingMessages_check_unit_config_tag;
    strcpy(gSendMessage.SMIncomingMessage.check_unit_config.vendor_version, testVersion);
    {
        UniqueLock lock(gReadMutex);
        gReadyToRead = true;
        gReadCondVar.NotifyOne();
    }

    // Wait node config status
    {
        UniqueLock lock(gWaitMessageMutex);
        gWaitMessageCondVar.Wait([&] { return gReadyToCheck; });
        gReadyToCheck = false;
    }

    zassert_equal(gReceivedMessage.which_SMOutgoingMessage, servicemanager_v3_SMOutgoingMessages_unit_config_status_tag,
        "Unit configuration status expected");
    zassert_false((0 != strcmp(gReceivedMessage.SMOutgoingMessage.unit_config_status.vendor_version, testVersion)),
        "Incorrect unit config version");
}

void testUpdateUnitConfig()
{
    gSendMessage.which_SMIncomingMessage = servicemanager_v3_SMIncomingMessages_set_unit_config_tag;
    strcpy(gSendMessage.SMIncomingMessage.set_unit_config.vendor_version, testVersion);
    {
        UniqueLock lock(gReadMutex);
        gReadyToRead = true;
        gReadCondVar.NotifyOne();
    }

    // Wait node config status
    {
        UniqueLock lock(gWaitMessageMutex);
        gWaitMessageCondVar.Wait([&] { return gReadyToCheck; });
        gReadyToCheck = false;
    }

    zassert_equal(gReceivedMessage.which_SMOutgoingMessage, servicemanager_v3_SMOutgoingMessages_unit_config_status_tag,
        "Unit configuration status expected");

    zassert_false((0 != strcmp(gReceivedMessage.SMOutgoingMessage.unit_config_status.vendor_version, testVersion)),
        "Incorrect unit config version");
}

void testRunInstances()
{
    gSendMessage.which_SMIncomingMessage = servicemanager_v3_SMIncomingMessages_run_instances_tag;
    gSendMessage.SMIncomingMessage.run_instances
        = servicemanager_v3_RunInstances servicemanager_v3_RunInstances_init_zero;

    gSendMessage.SMIncomingMessage.run_instances.force_restart = true;
    gMockLauncher.mExpectedForceRestart = true;

    gSendMessage.SMIncomingMessage.run_instances.services_count = 1;

    auto&            pbService = gSendMessage.SMIncomingMessage.run_instances.services[0];
    aos::ServiceInfo expectedService;

    pbService.has_version_info = true;
    pbService.version_info.aos_version = 42;
    expectedService.mVersionInfo.mAosVersion = 42;
    strcpy(pbService.url, "Service1URL");
    expectedService.mURL = "Service1URL";
    strcpy(pbService.service_id, "ServiceID1");
    expectedService.mServiceID = aos::String("ServiceID1");
    strcpy(pbService.url, "URLServiceID1");
    expectedService.mURL = aos::String("URLServiceID1");

    gMockLauncher.mExpectedServices.PushBack(expectedService);

    gSendMessage.SMIncomingMessage.run_instances.instances_count = 1;

    auto&             pbInstance = gSendMessage.SMIncomingMessage.run_instances.instances[0];
    aos::InstanceInfo expectedInstance;

    pbInstance.has_instance = true;
    pbInstance.instance.instance = 1;
    expectedInstance.mInstanceIdent.mInstance = 1;
    strcpy(pbInstance.instance.service_id, "InstanceService1");
    expectedInstance.mInstanceIdent.mServiceID = "InstanceService1";
    strcpy(pbInstance.instance.subject_id, "Subject1");
    expectedInstance.mInstanceIdent.mSubjectID = "Subject1";
    pbInstance.priority = 42;
    expectedInstance.mPriority = 42;

    gMockLauncher.mExpectedInstances.PushBack(expectedInstance);

    {
        UniqueLock lock(gReadMutex);
        gReadyToRead = true;
        gReadCondVar.NotifyOne();
    }

    {
        UniqueLock lock(gWaitMessageMutex);
        gWaitMessageCondVar.Wait([&] { return gReadyToCheck; });
        gReadyToCheck = false;
    }
}

void testImageDownloadInstances()
{
    printk("[test] ----------- GetUnitConfigInfo ---------------- \n");
    ImageContentRequest request;
    request.contentType = DownloadContentType::Enum::eService;
    request.requestID = 100500;
    request.url = aos::StaticString<50>("/file/url");

    gTestCMClient.SendImageContentRequest(request);

    {
        UniqueLock lock(gWaitMessageMutex);
        gWaitMessageCondVar.Wait([&] { return gReadyToCheck; });
        gReadyToCheck = false;
    }

    zassert_equal(gReceivedMessage.which_SMOutgoingMessage,
        servicemanager_v3_SMOutgoingMessages_image_content_request_tag, "Image content request expected");
    zassert_equal(
        gReceivedMessage.SMOutgoingMessage.image_content_request.request_id, request.requestID, "Incorrect request id");
    zassert_equal(aos::String(gReceivedMessage.SMOutgoingMessage.image_content_request.content_type), "service",
        "Incorrect request content type");
    zassert_equal(
        aos::String(gReceivedMessage.SMOutgoingMessage.image_content_request.url), request.url.CStr(), "Incorrect url");

    gSendMessage.which_SMIncomingMessage = servicemanager_v3_SMIncomingMessages_image_content_info_tag;
    gSendMessage.SMIncomingMessage.image_content_info
        = servicemanager_v3_ImageContentInfo servicemanager_v3_ImageContentInfo_init_zero;
    gSendMessage.SMIncomingMessage.image_content_info.request_id = 100500;
    gMockDownloader.mExpectedContent.requestID = 100500;
    gSendMessage.SMIncomingMessage.image_content_info.image_files_count = 1;

    FileInfo                     expectedFileInfo;
    servicemanager_v3_ImageFile& imageFile = gSendMessage.SMIncomingMessage.image_content_info.image_files[0];

    expectedFileInfo.size = 6400000;
    imageFile.size = 6400000;
    expectedFileInfo.relativePath = "imagePath";
    strcpy(imageFile.relative_path, "imagePath");
    memcpy(expectedFileInfo.sha256.Get(), "SHA256", 7);
    memcpy(imageFile.sha256.bytes, "SHA256", 7);
    imageFile.sha256.size = 7;

    gMockDownloader.mExpectedContent.files.PushBack(expectedFileInfo);

    {
        UniqueLock lock(gReadMutex);
        gReadyToRead = true;
        gReadCondVar.NotifyOne();
    }

    {
        UniqueLock lock(gWaitMessageMutex);
        gWaitMessageCondVar.Wait([&] { return gReadyToCheck; });
        gReadyToCheck = false;
    }

    gSendMessage.which_SMIncomingMessage = servicemanager_v3_SMIncomingMessages_image_content_tag;
    gSendMessage.SMIncomingMessage.image_content
        = servicemanager_v3_ImageContent servicemanager_v3_ImageContent_init_zero;

    gSendMessage.SMIncomingMessage.image_content.request_id = 100501;
    gMockDownloader.mExpectedChunk.requestID = 100501;
    gSendMessage.SMIncomingMessage.image_content.parts_count = 10;
    gMockDownloader.mExpectedChunk.partsCount = 10;
    gSendMessage.SMIncomingMessage.image_content.part = 2;
    gMockDownloader.mExpectedChunk.part = 2;
    strcpy(gSendMessage.SMIncomingMessage.image_content.relative_path, "Some/Path");
    gMockDownloader.mExpectedChunk.relativePath = "Some/Path";
    gSendMessage.SMIncomingMessage.image_content.data.size = strlen("File content");
    strcpy((char*)gSendMessage.SMIncomingMessage.image_content.data.bytes, "File content");
    gMockDownloader.mExpectedChunk.data = aos::Array<uint8_t>(gSendMessage.SMIncomingMessage.image_content.data.bytes,
        gSendMessage.SMIncomingMessage.image_content.data.size);

    {
        UniqueLock lock(gReadMutex);
        gReadyToRead = true;
        gReadCondVar.NotifyOne();
    }

    {
        UniqueLock lock(gWaitMessageMutex);
        gWaitMessageCondVar.Wait([&] { return gReadyToCheck; });
        gReadyToCheck = false;
    }
}

ZTEST_SUITE(cmclient, NULL, NULL, NULL, NULL, NULL);

ZTEST(cmclient, test_node_config)
{
    ResourceManager mockResourceManager;

    aos::Log::SetCallback(TestLogCallback);

    auto err = gTestCMClient.Init(gMockLauncher, mockResourceManager, gMockDownloader);
    zassert_true(err.IsNone(), "init error: %s", err.Message());

    // wait open
    {
        UniqueLock lock(gWaitMessageMutex);
        gWaitMessageCondVar.Wait([&] { return gReadyToCheck; });
        gReadyToCheck = false;
    }

    {
        UniqueLock lock(gReadMutex);
        gReadyToRead = true;
        gReadCondVar.NotifyOne();
    }

    // Wait node config
    {
        UniqueLock lock(gWaitMessageMutex);
        gWaitMessageCondVar.Wait([&] { return gReadyToCheck; });
        gReadyToCheck = false;
    }

    zassert_equal(gReceivedMessage.which_SMOutgoingMessage, servicemanager_v3_SMOutgoingMessages_node_configuration_tag,
        "Node configuration expected");

    tesUpdateAndRunInstanceStatuses();
    testGetUnitConfigStatus();
    testCheckUnitConfig();
    testUpdateUnitConfig();
    testRunInstances();
    testImageDownloadInstances();

    // release read tread to shutdown
    UniqueLock lock(gReadMutex);
    gReadyToRead = true;
    gShutdown = true;
    gReadCondVar.NotifyOne();
}
