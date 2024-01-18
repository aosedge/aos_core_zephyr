/*
 * Copyright (C) 2024 Renesas Electronics Corporation.
 * Copyright (C) 2024 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <pb_decode.h>
#include <vchanapi.h>
#include <zephyr/sys/printk.h>

#include "communication/messagehandler.hpp"
#include "utils/checksum.hpp"

#include "utils.hpp"

/***********************************************************************************************************************
 * Public
 **********************************************************************************************************************/

void TestLogCallback(aos::LogModule module, aos::LogLevel level, const aos::String& message)
{
    static std::mutex mutex;
    static auto       startTime = std::chrono::steady_clock::now();

    std::lock_guard<std::mutex> lock(mutex);

    auto now = std::chrono::duration<float>(std::chrono::steady_clock::now() - startTime).count();

    const char* levelStr = "unknown";

    switch (level.GetValue()) {
    case aos::LogLevelEnum::eDebug:
        levelStr = "dbg";
        break;

    case aos::LogLevelEnum::eInfo:
        levelStr = "inf";
        break;

    case aos::LogLevelEnum::eWarning:
        levelStr = "wrn";
        break;

    case aos::LogLevelEnum::eError:
        levelStr = "err";
        break;

    default:
        levelStr = "n/d";
        break;
    }

    printk("%0.3f [%s] %s\n", now, levelStr, message.CStr());
}

aos::Error SendMessageToChannel(CommChannelMock& channel, uint32_t source, const std::string& methodName,
    uint64_t requestID, const std::vector<uint8_t>& data, aos::Error messageError)
{
    auto checksum = CalculateSha256(aos::Array<uint8_t>(data.data(), data.size()));
    if (!checksum.mError.IsNone()) {
        return AOS_ERROR_WRAP(checksum.mError);
    }

    auto header = VChanMessageHeader {source, static_cast<uint32_t>(data.size()), requestID, messageError.Errno(),
        static_cast<int32_t>(messageError.Value())};

    strncpy(header.mMethodName, methodName.c_str(), cMaxMethodLen);

    aos::Array<uint8_t>(header.mSha256, aos::cSHA256Size) = checksum.mValue;

    auto headerPtr = reinterpret_cast<uint8_t*>(&header);

    channel.SendRead(std::vector<uint8_t>(headerPtr, headerPtr + sizeof(VChanMessageHeader)));
    channel.SendRead(data);

    return aos::ErrorEnum::eNone;
}

aos::Error ReceiveMessageFromChannel(CommChannelMock& channel, uint32_t source, const std::string& methodName,
    uint64_t requestID, std::vector<uint8_t>& data)
{
    std::vector<uint8_t> headerData;

    auto err = channel.WaitWrite(headerData, sizeof(VChanMessageHeader), cWaitTimeout);
    if (!err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    auto header = reinterpret_cast<VChanMessageHeader*>(headerData.data());

    if (header->mSource != source) {
        return AOS_ERROR_WRAP(aos::ErrorEnum::eInvalidArgument);
    }

    if (header->mMethodName != methodName) {
        return AOS_ERROR_WRAP(aos::ErrorEnum::eInvalidArgument);
    }

    if (header->mRequestID != requestID) {
        return AOS_ERROR_WRAP(aos::ErrorEnum::eInvalidArgument);
    }

    err = channel.WaitWrite(data, header->mDataSize, cWaitTimeout);
    if (!err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    if (header->mDataSize) {
        auto checksum = CalculateSha256(aos::Array<uint8_t>(data.data(), data.size()));
        if (!checksum.mError.IsNone()) {
            return AOS_ERROR_WRAP(checksum.mError);
        }

        if (checksum.mValue != aos::Array<uint8_t>(header->mSha256, aos::cSHA256Size)) {
            return AOS_ERROR_WRAP(aos::ErrorEnum::eInvalidChecksum);
        }
    }

    if (header->mAosError) {
        return AOS_ERROR_WRAP(static_cast<aos::ErrorEnum>(header->mAosError));
    }

    if (header->mErrno) {
        return AOS_ERROR_WRAP(header->mAosError);
    }

    return aos::ErrorEnum::eNone;
}

aos::Error SendPBMessage(CommChannelMock& channel, uint32_t source, const std::string& methodName, uint64_t requestID,
    const pb_msgdesc_t* fields, const void* message, size_t messageSize)
{
    std::vector<uint8_t> data(messageSize);

    if (messageSize) {
        auto stream = pb_ostream_from_buffer(data.data(), data.size());

        auto status = pb_encode(&stream, fields, message);
        if (!status) {
            return aos::ErrorEnum::eFailed;
        }

        data.resize(stream.bytes_written);
    }

    return SendMessageToChannel(channel, source, methodName, requestID, data);
}

aos::Error ReceivePBMessage(CommChannelMock& channel, uint32_t source, const std::string& methodName,
    uint64_t requestID, const pb_msgdesc_t* fields, void* message, size_t messageSize)
{
    std::vector<uint8_t> data(messageSize);

    auto err = ReceiveMessageFromChannel(channel, source, methodName, requestID, data);
    if (!err.IsNone()) {
        return err;
    }

    if (message && fields) {
        auto stream = pb_istream_from_buffer(data.data(), data.size());

        auto status = pb_decode(&stream, fields, message);
        if (!status) {
            return AOS_ERROR_WRAP(aos::ErrorEnum::eRuntime);
        }
    }

    return aos::ErrorEnum::eNone;
}
