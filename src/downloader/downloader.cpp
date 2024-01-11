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

/***********************************************************************************************************************
 * Public
 **********************************************************************************************************************/

Downloader::~Downloader()
{
    aos::LockGuard lock(mMutex);

    mFinishDownload = true;
    mWaitDownload.NotifyOne();
}

aos::Error Downloader::Init(DownloadRequesterItf& downloadRequester)
{
    LOG_DBG() << "Initialize downloader";

    mDownloadRequester = &downloadRequester;

    return aos::ErrorEnum::eNone;
}

aos::Error Downloader::Download(const aos::String& url, const aos::String& path, aos::DownloadContent contentType)
{
    aos::UniqueLock lock(mMutex);

    LOG_DBG() << "Download: " << url;

    mFinishDownload = false;
    mDownloadResults.Clear();

    auto err = mTimer.Create(
        Downloader::cDownloadTimeout, [this](void*) { SetErrorAndNotify(AOS_ERROR_WRAP(aos::ErrorEnum::eTimeout)); });
    if (!err.IsNone()) {
        LOG_DBG() << "Create timer failed: " << err;

        return AOS_ERROR_WRAP(err);
    }

    mRequestedPath          = path;
    mErrProcessImageRequest = mDownloadRequester->SendImageContentRequest({url, ++mRequestID, contentType});

    if (!mErrProcessImageRequest.IsNone()) {
        return mErrProcessImageRequest;
    }

    mWaitDownload.Wait(lock, [this] { return mFinishDownload; });

    err = mTimer.Stop();
    if (!err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    for (const auto& result : mDownloadResults) {
        if (result.mFile == -1) {
            continue;
        }

        auto ret = close(result.mFile);
        if (ret < 0) {
            err = AOS_ERROR_WRAP(errno);

            LOG_ERR() << "Can't close file. Path: " << result.mRelativePath << ", err: " << err;

            if (mErrProcessImageRequest.IsNone()) {
                mErrProcessImageRequest = err;
            }
        }
    }

    return mErrProcessImageRequest;
}

aos::Error Downloader::ReceiveFileChunk(const FileChunk& chunk)
{
    LOG_DBG() << "Receive file chunk: path = " << chunk.mRelativePath << ", chunk = " << chunk.mPart << "/"
              << chunk.mPartsCount;

    aos::LockGuard lock(mMutex);

    auto downloadResult = mDownloadResults.Find(
        [&chunk](const DownloadResult& result) { return result.mRelativePath == chunk.mRelativePath; });

    if (!downloadResult.mError.IsNone()) {
        auto err = AOS_ERROR_WRAP(downloadResult.mError);

        SetErrorAndNotify(err);

        return err;
    }

    if (downloadResult.mValue->mFile == -1) {
        auto path    = aos::FS::JoinPath(mRequestedPath, chunk.mRelativePath);
        auto dirPath = aos::FS::Dir(path);

        auto err = aos::FS::MakeDirAll(dirPath);
        if (!err.IsNone()) {
            err = AOS_ERROR_WRAP(errno);

            SetErrorAndNotify(err);

            return err;
        }

        downloadResult.mValue->mFile = open(path.CStr(), O_CREAT | O_WRONLY, 0644);
        if (downloadResult.mValue->mFile < 0) {
            err = AOS_ERROR_WRAP(errno);

            SetErrorAndNotify(err);

            return err;
        }
    }

    auto ret = write(downloadResult.mValue->mFile, chunk.mData.Get(), chunk.mData.Size());
    if (ret < 0) {
        auto err = AOS_ERROR_WRAP(errno);

        SetErrorAndNotify(err);

        return err;
    }

    mTimer.Reset([this](void*) { SetErrorAndNotify(AOS_ERROR_WRAP(aos::ErrorEnum::eTimeout)); });

    if (chunk.mPart == chunk.mPartsCount) {
        ret = close(downloadResult.mValue->mFile);
        if (ret < 0) {
            auto err = AOS_ERROR_WRAP(errno);

            SetErrorAndNotify(err);

            return err;
        }

        downloadResult.mValue->mFile   = -1;
        downloadResult.mValue->mIsDone = true;

        if (IsAllDownloadDone()) {
            mFinishDownload = true;
            mWaitDownload.NotifyOne();
        }
    }

    return aos::ErrorEnum::eNone;
}

aos::Error Downloader::ReceiveImageContentInfo(const ImageContentInfo& content)
{
    aos::LockGuard lock(mMutex);

    LOG_DBG() << "Receive image content info";

    if (content.mRequestID != mRequestID) {
        return AOS_ERROR_WRAP(aos::ErrorEnum::eFailed);
    }

    if (content.mError != "") {
        LOG_ERR() << "Error: " << content.mError;

        auto err = AOS_ERROR_WRAP(aos::ErrorEnum::eFailed);

        SetErrorAndNotify(err);

        return err;
    }

    for (auto& file : content.mFiles) {
        mDownloadResults.PushBack(DownloadResult {file.mRelativePath, -1, false});
    }

    mTimer.Reset([this](void*) { SetErrorAndNotify(AOS_ERROR_WRAP(aos::ErrorEnum::eTimeout)); });

    return aos::ErrorEnum::eNone;
}

/***********************************************************************************************************************
 * Private
 **********************************************************************************************************************/

bool Downloader::IsAllDownloadDone() const
{
    for (auto& result : mDownloadResults) {
        if (!result.mIsDone) {
            return false;
        }
    }

    return true;
}

void Downloader::SetErrorAndNotify(const aos::Error& err)
{
    mFinishDownload         = true;
    mErrProcessImageRequest = err;
    mWaitDownload.NotifyOne();
}
