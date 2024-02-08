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
    size_t read = 0;

    auto err = data.Resize(size);
    if (!err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    while (read < size) {
        auto ret = vch_read(&mReadHandle, data.Get() + read, size - read);
        if (ret < 0) {
            return AOS_ERROR_WRAP(ret);
        }

        read += ret;
    }

    assert(read <= data.MaxSize());

    if (read != size) {
        return AOS_ERROR_WRAP(aos::ErrorEnum::eFailed);
    }

    return aos::ErrorEnum::eNone;
}

aos::Error VChannel::Write(const aos::Array<uint8_t>& data)
{
    size_t written = 0;

    while (written < data.Size()) {
        auto ret = vch_write(&mWriteHandle, data.Get() + written, data.Size() - written);
        if (ret < 0) {
            return AOS_ERROR_WRAP(ret);
        }

        written += ret;
    }

    if (written != data.Size()) {
        return AOS_ERROR_WRAP(aos::ErrorEnum::eFailed);
    }

    return aos::ErrorEnum::eNone;
}
