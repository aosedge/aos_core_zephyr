/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <fcntl.h>
#include <sys/stat.h>

#include <aos/common/tools/fs.hpp>

#include "downloader.hpp"
#include "log.hpp"

Downloader::~Downloader()
{
    aos::LockGuard lock(mMutex);
    mFinishDownload = true;
    mWaitDownload.NotifyOne();
}

aos::Error Downloader::Init(DownloadRequesterItf& downloadRequester)
{
    LOG_DBG() << "Initialize CM client";

    mDownloadRequester = &downloadRequester;

    return aos::ErrorEnum::eNone;
}

aos::Error Downloader::Download(const aos::String& url, const aos::String& path, aos::DownloadContent contentType)
{
    LOG_DBG() << "Download: " << url;

    aos::LockGuard lock(mMutex);
    mFinishDownload = false;
    mDownloadResults.Clear();

    auto err
        = mTimer.Create(Downloader::cDownloadTimeout, [this](void*) { SetErrorAndNotify(aos::ErrorEnum::eTimeout); });
    if (!err.IsNone()) {
        LOG_DBG() << "Create timer failed " << strerror(errno);
        return AOS_ERROR_WRAP(err);
    }

    mRequestedPath = path;
    mErrProcessImageRequest = mDownloadRequester->SendImageContentRequest({url, ++mRequestID, contentType});

    if (!mErrProcessImageRequest.IsNone()) {
        return mErrProcessImageRequest;
    }

    mWaitDownload.Wait([this] { return mFinishDownload; });

    err = mTimer.Stop();
    if (!err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    return mErrProcessImageRequest;
}

aos::Error Downloader::ReceiveFileChunk(const FileChunk& chunk)
{
    LOG_DBG() << "Receive file chunk";

    aos::LockGuard lock(mMutex);

    auto result
        = mDownloadResults
              .Find([&chunk](const DownloadResult& result) { return result.mRelativePath == chunk.relativePath; })
              .mValue;

    if (result == nullptr) {
        LOG_ERR() << "File chunk for unknown file: " << chunk.relativePath;
        SetErrorAndNotify(aos::ErrorEnum::eNotFound);

        return aos::ErrorEnum::eNotFound;
    }

    if (result->mFile == -1) {
        aos::StaticString<aos::cFilePathLen> path;
        path.Append(mRequestedPath).Append("/").Append(chunk.relativePath);

        auto dirPath = aos::FS::Dir(path);

        auto err = aos::FS::MakeDir(dirPath);
        if (!err.IsNone()) {
            LOG_ERR() << "Failed to create directory: " << dirPath;
            SetErrorAndNotify(err);

            return err;
        }

        result->mFile = open(path.CStr(), O_CREAT | O_WRONLY, 0644);
        if (result->mFile < 0) {
            auto err = AOS_ERROR_WRAP(errno);
            SetErrorAndNotify(err);

            return err;
        }
    }

    auto ret = write(result->mFile, chunk.data.Get(), chunk.data.Size());
    if (ret < 0) {
        auto err = AOS_ERROR_WRAP(ret);
        SetErrorAndNotify(err);

        return err;
    }

    mTimer.Reset([this](void*) { SetErrorAndNotify(aos::ErrorEnum::eTimeout); });

    if (chunk.part == chunk.partsCount) {
        ret = close(result->mFile);
        if (ret < 0) {
            auto err = AOS_ERROR_WRAP(ret);
            SetErrorAndNotify(err);

            return err;
        }

        result->mFile = -1;
        result->mIsDone = true;

        if (IsAllDownloadDone()) {
            mFinishDownload = true;
            mWaitDownload.NotifyOne();
        }
    }

    return aos::ErrorEnum::eNone;
}

void Downloader::SetErrorAndNotify(const aos::Error& err)
{
    mFinishDownload = true;
    mErrProcessImageRequest = err;
    mWaitDownload.NotifyOne();
}

aos::Error Downloader::ReceiveImageContentInfo(const ImageContentInfo& content)
{
    LOG_DBG() << "Receive image content info";

    if (content.requestID != mRequestID) {
        return aos::ErrorEnum::eFailed;
    }

    if (content.error != "") {
        LOG_ERR() << "Error: " << content.error;

        SetErrorAndNotify(aos::ErrorEnum::eFailed);

        return aos::ErrorEnum::eFailed;
    }

    for (auto& file : content.files) {
        mDownloadResults.PushBack(DownloadResult {file.relativePath, -1, false});
    }

    mTimer.Reset([this](void*) { SetErrorAndNotify(aos::ErrorEnum::eTimeout); });

    return aos::ErrorEnum::eNone;
}

bool Downloader::IsAllDownloadDone() const
{
    for (auto& result : mDownloadResults) {
        if (!result.mIsDone) {
            return false;
        }
    }

    return true;
}
