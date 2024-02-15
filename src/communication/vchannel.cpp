/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vchannel.hpp"
#include "log.hpp"

/***********************************************************************************************************************
 * Public
 **********************************************************************************************************************/

aos::Error VChannel::Init(const aos::String& name, const aos::String& xsReadPath, const aos::String& xsWritePath)
{
    mXSReadPath  = xsReadPath;
    mXSWritePath = xsWritePath;
    mName        = name;

    return aos::ErrorEnum::eNone;
}

aos::Error VChannel::SetTLSConfig(const aos::String& certType)
{
    if (certType != "") {
        return aos::ErrorEnum::eNotSupported;
    }

    return aos::ErrorEnum::eNone;
}

aos::Error VChannel::Connect()
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

    mConnected = true;

    return aos::ErrorEnum::eNone;
}

aos::Error VChannel::Close()
{
    vch_close(&mReadHandle);
    vch_close(&mWriteHandle);

    mConnected = false;

    return aos::ErrorEnum::eNone;
}

int VChannel::Read(void* data, size_t size)
{
    LOG_DBG() << "Channel wants read: channel = " << mName << ", size = " << size;

    auto ret = vch_read(&mReadHandle, data, size);

    LOG_DBG() << "Channel read: channel = " << mName << ", size = " << ret;

    return ret;
}

int VChannel::Write(const void* data, size_t size)
{
    LOG_DBG() << "Channel wants write: channel = " << mName << ", size = " << size;

    auto ret = vch_write(&mWriteHandle, data, size);

    LOG_DBG() << "Channel wrote: channel = " << mName << ", size = " << size;

    return ret;
}
