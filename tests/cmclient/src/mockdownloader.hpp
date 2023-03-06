/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "downloader/downloader.hpp"

class MockDownloader : public DownloadReceiverItf {
public:
    MockDownloader(aos::ConditionalVariable& waitMessageCondVar, aos::Mutex& waitMessageMutex, bool& readyToProcess)
        : mWaitMessageCondVar(waitMessageCondVar)
        , mWaitMessageMutex(waitMessageMutex)
        , mReadyToProcess(readyToProcess)
    {
    }

    aos::Error ReceiveFileChunk(const FileChunk& chunk) override;

    aos::Error ReceiveImageContentInfo(const ImageContentInfo& content) override;

    ImageContentInfo mExpectedContent;
    FileChunk        mExpectedChunk;

private:
    aos::ConditionalVariable& mWaitMessageCondVar;
    aos::Mutex&               mWaitMessageMutex;
    bool&                     mReadyToProcess;
};
