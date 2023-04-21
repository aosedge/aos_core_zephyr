/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/tc_util.h>
#include <zephyr/ztest.h>

#include <aos/common/tools/thread.hpp>

#include "mockdownloader.hpp"

aos::Error MockDownloader::ReceiveFileChunk(const FileChunk& chunk)
{
    zassert_equal(mExpectedChunk.relativePath, chunk.relativePath, "Incorrect rel path exp: %s rec:%s",
        mExpectedChunk.relativePath.CStr(), chunk.relativePath.CStr());
    zassert_equal(mExpectedChunk.part, chunk.part, "Incorrect part");
    zassert_equal(mExpectedChunk.partsCount, chunk.partsCount, "Incorrect part count");
    zassert_equal(mExpectedChunk.requestID, chunk.requestID, "Incorrect request id");
    zassert_false(memcmp(mExpectedChunk.data.begin(), chunk.data.begin(), chunk.data.Size()), "Incorrect chunk data");

    aos::UniqueLock lock(mWaitMessageMutex);
    mReadyToProcess = true;
    mWaitMessageCondVar.NotifyOne();

    return aos::ErrorEnum::eNone;
};

aos::Error MockDownloader::ReceiveImageContentInfo(const ImageContentInfo& content)
{
    zassert_equal(mExpectedContent.error, content.error, "Incorrect error msg");
    zassert_equal(mExpectedContent.requestID, content.requestID, "Incorrect request ID");
    zassert_equal(mExpectedContent.files.Size(), content.files.Size(), "Incorrect count of files");
    zassert_equal(mExpectedContent.files[0].relativePath, content.files[0].relativePath, "Incorrect rel path");
    zassert_false(strcmp((char*)mExpectedContent.files[0].sha256.Get(), (char*)content.files[0].sha256.Get()),
        "Incorrect sha256");

    aos::UniqueLock lock(mWaitMessageMutex);
    mReadyToProcess = true;
    mWaitMessageCondVar.NotifyOne();

    return aos::ErrorEnum::eNone;
};
