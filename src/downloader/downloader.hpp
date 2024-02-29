/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef DOWNLOADER_HPP_
#define DOWNLOADER_HPP_

#include <aos/common/downloader.hpp>
#include <aos/common/tools/array.hpp>
#include <aos/common/tools/error.hpp>
#include <aos/common/tools/string.hpp>
#include <aos/common/tools/thread.hpp>
#include <aos/common/tools/timer.hpp>
#include <aos/common/types.hpp>

/**
 * Image content request.
 */
struct ImageContentRequest {
    aos::StaticString<aos::cURLLen> mURL;
    uint64_t                        mRequestID;
    aos::DownloadContent            mContentType;

    /**
     * Compares image content request.
     *
     * @param request request to compare.
     * @return bool.
     */
    bool operator==(const ImageContentRequest& request) const
    {
        return mURL == request.mURL && mRequestID == request.mRequestID && mContentType == request.mContentType;
    }

    /**
     * Compares image content request.
     *
     * @param request request to compare.
     * @return bool.
     */
    bool operator!=(const ImageContentRequest& request) const { return !operator==(request); }
};

/**
 * File info.
 */
struct FileInfo {
    aos::StaticString<aos::cFilePathLen>        mRelativePath;
    aos::StaticArray<uint8_t, aos::cSHA256Size> mSHA256;
    uint64_t                                    mSize;

    /**
     * Compares file info.
     *
     * @param info info to compare.
     * @return bool.
     */
    bool operator==(const FileInfo& info) const
    {
        return mRelativePath == info.mRelativePath && mSHA256 == info.mSHA256 && mSize == info.mSize;
    }

    /**
     * Compares file info.
     *
     * @param info info to compare.
     * @return bool.
     */
    bool operator!=(const FileInfo& info) const { return !operator==(info); }
};

/**
 * Image content info.
 */
struct ImageContentInfo {
    uint64_t                                 mRequestID;
    aos::StaticArray<FileInfo, 32>           mFiles;
    aos::StaticString<aos::cErrorMessageLen> mError;

    /**
     * Compares content info.
     *
     * @param info info to compare.
     * @return bool.
     */
    bool operator==(const ImageContentInfo& info) const
    {
        return mRequestID == info.mRequestID && mFiles == info.mFiles && mError == info.mError;
    }

    /**
     * Compares content info.
     *
     * @param info info to compare.
     * @return bool.
     */
    bool operator!=(const ImageContentInfo& info) const { return !operator==(info); }
};

/**
 * File chunk.
 */
struct FileChunk {
    uint64_t                                       mRequestID;
    aos::StaticString<aos::cFilePathLen>           mRelativePath;
    uint64_t                                       mPartsCount;
    uint64_t                                       mPart;
    aos::StaticArray<uint8_t, aos::cFileChunkSize> mData;

    /**
     * Compares file chunks.
     *
     * @param chunk chunk to compare.
     * @return bool.
     */
    bool operator==(const FileChunk& chunk) const
    {
        return mRequestID == chunk.mRequestID && mRelativePath == chunk.mRelativePath
            && mPartsCount == chunk.mPartsCount && mPart == chunk.mPart && mData == chunk.mData;
    }

    /**
     * Compares file chunks.
     *
     * @param chunk chunk to compare.
     * @return bool.
     */
    bool operator!=(const FileChunk& chunk) const { return !operator==(chunk); }
};

/**
 * Download requester interface.
 */
class DownloadRequesterItf {
public:
    /**
     * Sends image content request.
     *
     * @param request image content request
     * @return Error
     */
    virtual aos::Error SendImageContentRequest(const ImageContentRequest& request) = 0;

    /**
     * Destroys the Download Requester Itf object.
     */
    virtual ~DownloadRequesterItf() = default;
};

/**
 * @brief Download receiver interface
 *
 */
class DownloadReceiverItf {
public:
    /**
     * Receives image content.
     *
     * @param chunk file chunk
     * @return Error
     */
    virtual aos::Error ReceiveFileChunk(const FileChunk& chunk) = 0;

    /**
     * Receives image content info.
     *
     * @param content image content info
     * @return Error
     */
    virtual aos::Error ReceiveImageContentInfo(const ImageContentInfo& content) = 0;

    /**
     * Destroys the Download Receiver Itf object.
     */
    virtual ~DownloadReceiverItf() = default;
};

/**
 * Downloader class.
 *
 */
class Downloader : public aos::DownloaderItf, public DownloadReceiverItf {
public:
    /**
     * Default constructor.
     */
    Downloader() = default;

    /**
     * Destroys the Downloader object.
     */
    ~Downloader();

    /**
     * Initializes downloader instance.
     *
     * @param downloadRequester download requester instance.
     * @return Error
     */
    aos::Error Init(DownloadRequesterItf& downloadRequester);

    /**
     * Downloads file.
     *
     * @param url URL.
     * @param path path to file.
     * @param contentType content type.
     * @return Error
     */
    aos::Error Download(const aos::String& url, const aos::String& path, aos::DownloadContent contentType) override;

    /**
     * Receives image content request.
     *
     * @param chunk file chunk
     * @return Error
     */
    aos::Error ReceiveFileChunk(const FileChunk& chunk) override;

    /**
     * Receives image content info.
     *
     * @param content image content info
     * @return Error
     */
    aos::Error ReceiveImageContentInfo(const ImageContentInfo& content) override;

private:
    static constexpr auto cDownloadTimeout = 10000;

    struct DownloadResult {
        aos::StaticString<aos::cFilePathLen> mRelativePath;
        int                                  mFile;
        bool                                 mIsDone;
    };

    bool IsAllDownloadDone() const;
    void SetErrorAndNotify(const aos::Error& err);

    DownloadRequesterItf*                mDownloadRequester {};
    aos::Mutex                           mMutex;
    aos::ConditionalVariable             mWaitDownload;
    aos::Error                           mErrProcessImageRequest {};
    aos::StaticString<aos::cFilePathLen> mRequestedPath {};
    aos::StaticArray<DownloadResult, 32> mDownloadResults {};
    aos::Timer                           mTimer {};
    uint64_t                             mRequestID {};
    bool                                 mFinishDownload {false};
};

#endif
