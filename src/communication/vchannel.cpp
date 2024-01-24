/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vchannel.hpp"

/***********************************************************************************************************************
 * Public
 **********************************************************************************************************************/

aos::Error VChannel::Init(const aos::String& xsReadPath, const aos::String xsWritePath)
{
    mXSReadPath  = xsReadPath;
    mXSWritePath = xsWritePath;

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

aos::Error VChannel::Read(aos::Array<uint8_t>& data, size_t size)
{
    auto err = data.Resize(size);
    if (!err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    auto ret = vch_read(&mReadHandle, data.Get(), size);
    if (ret < 0) {
        return AOS_ERROR_WRAP(ret);
    }

    if (static_cast<size_t>(ret) != size) {
        err = data.Resize(ret);
        if (!err.IsNone()) {
            return AOS_ERROR_WRAP(err);
        }

        return AOS_ERROR_WRAP(aos::ErrorEnum::eFailed);
    }

    return aos::ErrorEnum::eNone;
}

aos::Error VChannel::Write(const aos::Array<uint8_t>& data)
{
    auto ret = vch_write(&mWriteHandle, data.Get(), data.Size());
    if (ret < 0) {
        return AOS_ERROR_WRAP(ret);
    }

    if (static_cast<size_t>(ret) != data.Size()) {
        return AOS_ERROR_WRAP(aos::ErrorEnum::eFailed);
    }

    return aos::ErrorEnum::eNone;
}
