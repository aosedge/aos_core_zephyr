/*
 * Copyright (C) 2024 Renesas Electronics Corporation.
 * Copyright (C) 2024 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <fcntl.h>
#include <unistd.h>

#include "log.hpp"
#include "pipe.hpp"

namespace aos::zephyr::communication {

/***********************************************************************************************************************
 * Public
 **********************************************************************************************************************/

aos::Error Transport::Init(const aos::String& readPipePath, const aos::String& writePipePath)
{
    mReadPipePath  = readPipePath;
    mWritePipePath = writePipePath;

    return aos::ErrorEnum::eNone;
}

aos::Error Transport::Open()
{
    mReadFd = open(mReadPipePath.CStr(), O_RDONLY | O_NONBLOCK);
    if (mReadFd == -1) {
        return aos::Error(aos::ErrorEnum::eRuntime, "failed to open read pipe");
    }

    mWriteFd = open(mWritePipePath.CStr(), O_WRONLY);
    if (mWriteFd == -1) {
        close(mReadFd);
        return aos::Error(aos::ErrorEnum::eRuntime, "failed to open write pipe");
    }

    mOpened = true;

    return aos::ErrorEnum::eNone;
}

aos::Error Transport::Close()
{
    Error err;

    if (close(mReadFd) == -1) {
        err = aos::Error(aos::ErrorEnum::eRuntime, "failed to close read fd");
    }

    if (close(mWriteFd) == -1) {
        if (err.IsNone()) {
            err = aos::Error(aos::ErrorEnum::eRuntime, "failed to close write fd");
        }
    }

    mOpened = false;

    return err;
}

int Transport::Read(void* data, size_t size)
{
    return read(mReadFd, data, size);
}

int Transport::Write(const void* data, size_t size)
{
    return write(mWriteFd, data, size);
}

} // namespace aos::zephyr::communication
