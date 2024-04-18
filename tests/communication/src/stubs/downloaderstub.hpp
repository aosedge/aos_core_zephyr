/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef DOWNLOADERSTUB_HPP_
#define DOWNLOADERSTUB_HPP_

#include <condition_variable>
#include <mutex>

#include "downloader/downloader.hpp"

class DownloaderStub : public DownloadReceiverItf {
public:
    aos::Error ReceiveFileChunk(const FileChunk& chunk) override
    {
        std::lock_guard<std::mutex> lock(mMutex);

        mFileChunk     = chunk;
        mEventReceived = true;

        mCV.notify_one();

        return aos::ErrorEnum::eNone;
    }

    aos::Error ReceiveImageContentInfo(const ImageContentInfo& content) override
    {
        std::lock_guard<std::mutex> lock(mMutex);

        mContentInfo   = content;
        mEventReceived = true;

        mCV.notify_one();

        return aos::ErrorEnum::eNone;
    }

    const FileChunk& GetFileChunk() const
    {
        std::lock_guard<std::mutex> lock(mMutex);
        return mFileChunk;
    }

    const ImageContentInfo& GetContentInfo() const
    {
        std::lock_guard<std::mutex> lock(mMutex);
        return mContentInfo;
    }

    aos::Error WaitEvent(const std::chrono::duration<double> timeout)
    {
        std::unique_lock<std::mutex> lock(mMutex);

        if (!mCV.wait_for(lock, timeout, [&] { return mEventReceived; })) {
            return aos::ErrorEnum::eTimeout;
        }

        mEventReceived = false;

        return aos::ErrorEnum::eNone;
    }

    void Clear()
    {
        std::lock_guard<std::mutex> lock(mMutex);

        mEventReceived = false;
        mContentInfo   = ImageContentInfo();
        mFileChunk     = FileChunk();
    }

private:
    ImageContentInfo mContentInfo {};
    FileChunk        mFileChunk {};

    std::condition_variable mCV;
    mutable std::mutex      mMutex;
    bool                    mEventReceived = false;
};

#endif
