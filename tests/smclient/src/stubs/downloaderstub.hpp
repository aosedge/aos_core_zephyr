/*
 * Copyright (C) 2024 Renesas Electronics Corporation.
 * Copyright (C) 2024 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef DOWNLOADERSTUB_HPP_
#define DOWNLOADERSTUB_HPP_

#include <condition_variable>
#include <mutex>

#include "downloader/downloader.hpp"

class DownloaderStub : public aos::zephyr::downloader::DownloadReceiverItf {
public:
    aos::Error ReceiveFileChunk(const aos::zephyr::downloader::FileChunk& chunk) override
    {
        std::lock_guard<std::mutex> lock(mMutex);

        mFileChunk     = chunk;
        mEventReceived = true;

        mCV.notify_one();

        return aos::ErrorEnum::eNone;
    }

    aos::Error ReceiveImageContentInfo(const aos::zephyr::downloader::ImageContentInfo& content) override
    {
        std::lock_guard<std::mutex> lock(mMutex);

        mContentInfo   = content;
        mEventReceived = true;

        mCV.notify_one();

        return aos::ErrorEnum::eNone;
    }

    const aos::zephyr::downloader::FileChunk& GetFileChunk() const
    {
        std::lock_guard<std::mutex> lock(mMutex);
        return mFileChunk;
    }

    const aos::zephyr::downloader::ImageContentInfo& GetContentInfo() const
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
        mContentInfo   = aos::zephyr::downloader::ImageContentInfo();
        mFileChunk     = aos::zephyr::downloader::FileChunk();
    }

private:
    aos::zephyr::downloader::ImageContentInfo mContentInfo {};
    aos::zephyr::downloader::FileChunk        mFileChunk {};

    std::condition_variable mCV;
    mutable std::mutex      mMutex;
    bool                    mEventReceived = false;
};

#endif
