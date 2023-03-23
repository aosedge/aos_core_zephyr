/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/tc_util.h>
#include <zephyr/ztest.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <aos/common/tools/log.hpp>
#include <aos/common/tools/timer.hpp>

#include "downloader/downloader.hpp"

using namespace aos;

#define UNUSED(x) (void)(x)

static const char* kDownloadPath = "download";
static const char* kFileName = "test.txt";
static const char* kDownloadUrl = "http://www.example.com";
static const char  data[] = "file content";

static Downloader  downloader;

void               sendImageContentInfo(void*)
{
    aos::StaticArray<FileInfo, 32> files;
    FileInfo                       file {};
    file.relativePath = kFileName;

    files.PushBack(file);

    zassert_equal(downloader.ReceiveImageContentInfo(ImageContentInfo {1, files}), aos::ErrorEnum::eNone,
        "Failed to initialize downloader");

    aos::StaticArray<uint8_t, aos::cFileChunkSize> fileData {};
    fileData.Resize(sizeof(data));
    memcpy(fileData.Get(), data, sizeof(data));

    FileChunk chunk {1, file.relativePath, 1, 1, fileData};

    zassert_equal(downloader.ReceiveFileChunk(chunk), aos::ErrorEnum::eNone, "Failed to receive file chunk");
}

static aos::Timer timerReceive {};

class TestDownloadRequester : public DownloadRequesterItf {
public:
    TestDownloadRequester(bool skipSendRequest = false)
        : mSkipSendRequest(skipSendRequest)
    {
    }

    aos::Error SendImageContentRequest(const ImageContentRequest& request) override
    {
        if (mSkipSendRequest) {
            return aos::ErrorEnum::eNone;
        }

        if (request.url != kDownloadUrl) {
            return aos::ErrorEnum::eInvalidArgument;
        }

        if (request.contentType != aos::DownloadContentEnum::eService) {
            return aos::ErrorEnum::eInvalidArgument;
        }

        if (request.requestID != 1) {
            return aos::ErrorEnum::eInvalidArgument;
        }

        timerReceive.Create(200, sendImageContentInfo);

        return aos::ErrorEnum::eNone;
    }

private:
    bool mSkipSendRequest;
};

void TestLogCallback(LogModule module, LogLevel level, const aos::String& message)
{
    printk("[downloader] %s \n", message.CStr());
}

ZTEST_SUITE(downloader, NULL, NULL, NULL, NULL, NULL);

ZTEST(downloader, test_download_image)
{
    zassert_equal(mkdir(kDownloadPath, S_IRWXU | S_IRWXG | S_IRWXO), 0, "Failed to create mount point");

    aos::Log::SetCallback(TestLogCallback);

    TestDownloadRequester requester;

    zassert_equal(downloader.Init(requester), aos::ErrorEnum::eNone, "Failed to initialize downloader");

    aos::StaticString<aos::cURLLen>      url {kDownloadUrl};
    aos::StaticString<aos::cFilePathLen> path {kDownloadPath};

    zassert_equal(downloader.Download(url, path, aos::DownloadContentEnum::eService), aos::ErrorEnum::eNone,
        "Failed to download image");

    aos::StaticString<aos::cFilePathLen> filePath {kDownloadPath};
    filePath.Append("/").Append(kFileName);

    auto file = open(filePath.CStr(), O_RDONLY, 0644);
    zassert_false(file < 0, "Failed to open file");

    aos::StaticArray<uint8_t, aos::cFileChunkSize> fileData {};
    fileData.Resize(sizeof(data));
    auto ret = read(file, fileData.Get(), fileData.Size());
    close(file);

    zassert_equal(ret, sizeof(data), "Failed to read file");
    zassert_equal(memcmp(fileData.Get(), data, sizeof(data)), 0, "File content is not equal to expected");
}

ZTEST(downloader, test_timeout_download_image)
{
    aos::Log::SetCallback(TestLogCallback);

    TestDownloadRequester requester {true};

    zassert_equal(downloader.Init(requester), aos::ErrorEnum::eNone, "Failed to initialize downloader");

    aos::StaticString<aos::cURLLen>      url {kDownloadUrl};
    aos::StaticString<aos::cFilePathLen> path {kDownloadPath};

    zassert_equal(downloader.Download(url, path, aos::DownloadContentEnum::eService), aos::ErrorEnum::eTimeout,
        "Expected timeout error");
}
