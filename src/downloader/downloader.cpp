/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <aos/common/tools/fs.hpp>

#include "downloader.hpp"
#include "log.hpp"

namespace aos::zephyr::downloader {

/***********************************************************************************************************************
 * Public
 **********************************************************************************************************************/

Downloader::~Downloader()
{
    LockGuard lock(mMutex);

    mFinishDownload = true;
    mWaitDownload.NotifyOne();
}

Error Downloader::Init(DownloadRequesterItf& downloadRequester)
{
    LOG_DBG() << "Initialize downloader";

    mDownloadRequester = &downloadRequester;

    return ErrorEnum::eNone;
}

Error Downloader::Download(const String& url, const String& path, aos::downloader::DownloadContent contentType)
{
    UniqueLock lock(mMutex);

    LOG_DBG() << "Download: " << url;

    mFinishDownload = false;
    mDownloadResults.Clear();

    auto err = mTimer.Start(
        Downloader::cDownloadTimeout, [this](void*) { SetErrorAndNotify(AOS_ERROR_WRAP(ErrorEnum::eTimeout)); });
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

Error Downloader::ReceiveFileChunk(const FileChunk& chunk)
{
    LOG_DBG() << "Receive file chunk: path = " << chunk.mRelativePath << ", chunk = " << chunk.mPart << "/"
              << chunk.mPartsCount;

    LockGuard lock(mMutex);

    auto downloadResult = mDownloadResults.FindIf(
        [&chunk](const DownloadResult& result) { return result.mRelativePath == chunk.mRelativePath; });

    if (downloadResult == mDownloadResults.end()) {
        auto err = AOS_ERROR_WRAP(ErrorEnum::eNotFound);

        SetErrorAndNotify(err);

        return err;
    }

    if (downloadResult->mFile == -1) {
        auto path    = fs::JoinPath(mRequestedPath, chunk.mRelativePath);
        auto dirPath = fs::Dir(path);

        auto err = fs::MakeDirAll(dirPath);
        if (!err.IsNone()) {
            err = AOS_ERROR_WRAP(errno);

            SetErrorAndNotify(err);

            return err;
        }

        downloadResult->mFile = open(path.CStr(), O_CREAT | O_WRONLY, 0644);
        if (downloadResult->mFile < 0) {
            err = AOS_ERROR_WRAP(errno);

            SetErrorAndNotify(err);

            return err;
        }
    }

    auto ret = write(downloadResult->mFile, chunk.mData.Get(), chunk.mData.Size());
    if (ret < 0) {
        auto err = AOS_ERROR_WRAP(errno);

        SetErrorAndNotify(err);

        return err;
    }

    mTimer.Start(
        Downloader::cDownloadTimeout, [this](void*) { SetErrorAndNotify(AOS_ERROR_WRAP(ErrorEnum::eTimeout)); });

    if (chunk.mPart == chunk.mPartsCount) {
        ret = close(downloadResult->mFile);
        if (ret < 0) {
            auto err = AOS_ERROR_WRAP(errno);

            SetErrorAndNotify(err);

            return err;
        }

        downloadResult->mFile   = -1;
        downloadResult->mIsDone = true;

        if (IsAllDownloadDone()) {
            mFinishDownload = true;
            mWaitDownload.NotifyOne();
        }
    }

    return ErrorEnum::eNone;
}

Error Downloader::ReceiveImageContentInfo(const ImageContentInfo& content)
{
    LockGuard lock(mMutex);

    LOG_DBG() << "Receive image content info";

    if (content.mRequestID != mRequestID) {
        return AOS_ERROR_WRAP(ErrorEnum::eFailed);
    }

    if (!content.mError.IsNone()) {
        LOG_ERR() << "Image content info error: err=" << content.mError;

        SetErrorAndNotify(content.mError);

        return content.mError;
    }

    for (auto& file : content.mFiles) {
        mDownloadResults.PushBack(DownloadResult {file.mRelativePath, -1, false});
    }

    mTimer.Start(
        Downloader::cDownloadTimeout, [this](void*) { SetErrorAndNotify(AOS_ERROR_WRAP(ErrorEnum::eTimeout)); });

    return ErrorEnum::eNone;
}

/***********************************************************************************************************************
 * Private
 **********************************************************************************************************************/

bool Downloader::IsAllDownloadDone() const
{
    for (const auto& result : mDownloadResults) {
        if (!result.mIsDone) {
            return false;
        }
    }

    return true;
}

void Downloader::SetErrorAndNotify(const Error& err)
{
    mFinishDownload         = true;
    mErrProcessImageRequest = err;

    LOG_DBG() << "Set error and notify: " << err;
    mWaitDownload.NotifyOne();
}

} // namespace aos::zephyr::downloader
