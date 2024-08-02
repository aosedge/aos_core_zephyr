/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "xenvchan.hpp"
#include "log.hpp"

namespace aos::zephyr::communication {

/***********************************************************************************************************************
 * Public
 **********************************************************************************************************************/

aos::Error XenVChan::Init(const aos::String& xsReadPath, const aos::String& xsWritePath)
{
    mXSReadPath  = xsReadPath;
    mXSWritePath = xsWritePath;

    return aos::ErrorEnum::eNone;
}

aos::Error XenVChan::Open()
{
    auto ret = vch_connect(cDomdID, mXSReadPath.CStr(), &mReadHandle);
    if (ret != 0) {
        return AOS_ERROR_WRAP(ret);
    }

    ret = vch_connect(cDomdID, mXSWritePath.CStr(), &mWriteHandle);
    if (ret != 0) {
        vch_close(&mReadHandle);

        return AOS_ERROR_WRAP(ret);
    }

    mReadHandle.blocking  = true;
    mWriteHandle.blocking = true;

    mOpened = true;

    return aos::ErrorEnum::eNone;
}

aos::Error XenVChan::Close()
{
    vch_close(&mReadHandle);
    vch_close(&mWriteHandle);

    mOpened = false;

    return aos::ErrorEnum::eNone;
}

int XenVChan::Read(void* data, size_t size)
{
    auto ret = vch_read(&mReadHandle, data, size);

    return ret;
}

int XenVChan::Write(const void* data, size_t size)
{
    auto ret = vch_write(&mWriteHandle, data, size);

    return ret;
}

} // namespace aos::zephyr::communication
