/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vchan.hpp"

aos::Error Vchan::Connect(domid_t domdID, const aos::String& vchanPath)
{
    auto ret = vch_connect(domdID, vchanPath.CStr(), &mVchanHandle);
    if (ret != 0) {
        return AOS_ERROR_WRAP(ret);
    }

    return aos::ErrorEnum::eNone;
}

aos::Error Vchan::Init()
{
    return aos::ErrorEnum::eNone;
}

aos::Error Vchan::Read(uint8_t* buffer, size_t size)
{
    auto read = vch_read(&mVchanHandle, buffer, size);
    if (read < 0) {
        return AOS_ERROR_WRAP(read);
    }

    return aos::ErrorEnum::eNone;
}

aos::Error Vchan::Write(const uint8_t* buffer, size_t size)
{
    auto written = vch_write(&mVchanHandle, buffer, size);
    if (written < 0) {
        return AOS_ERROR_WRAP(written);
    }

    return aos::ErrorEnum::eNone;
}

void Vchan::Disconnect()
{
    vch_close(&mVchanHandle);
}

Vchan::~Vchan()
{
    Disconnect();
}

void Vchan::SetBlocking(bool blocking)
{
    mVchanHandle.blocking = blocking;
}
