/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "downloader.hpp"

aos::Error Downloader::Init(DownloadRequesterItf& downloadRequester)
{
    mDownloadRequester = &downloadRequester;

    return aos::ErrorEnum::eNone;
}

aos::Error Downloader::Download(const aos::String& url, const aos::String& path, aos::DownloadContent contentType)
{
    (void)url;
    (void)path;
    (void)contentType;

    return aos::ErrorEnum::eNone;
}

aos::Error Downloader::ReceiveFileChunk(const FileChunk& chunk)
{
    (void)chunk;

    return aos::ErrorEnum::eNone;
}

aos::Error Downloader::ReceiveImageContentInfo(const ImageContentInfo& content)
{
    (void)content;

    return aos::ErrorEnum::eNone;
}
