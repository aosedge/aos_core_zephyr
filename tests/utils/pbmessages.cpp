/*
 * Copyright (C) 2024 Renesas Electronics Corporation.
 * Copyright (C) 2024 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <pb_decode.h>
#include <pb_encode.h>

#include <aosprotocol.h>

#include "pbmessages.hpp"

/***********************************************************************************************************************
 * Static
 **********************************************************************************************************************/

static aos::Error SendMessageToChannel(ChannelStub* channel, const std::vector<uint8_t>& data)
{
    auto header    = AosProtobufHeader {static_cast<uint32_t>(data.size())};
    auto headerPtr = reinterpret_cast<uint8_t*>(&header);

    channel->SendRead(std::vector<uint8_t>(headerPtr, headerPtr + sizeof(AosProtobufHeader)));
    channel->SendRead(data);

    return aos::ErrorEnum::eNone;
}

static aos::Error ReceiveMessageFromChannel(
    ChannelStub* channel, const std::chrono::duration<double>& timeout, std::vector<uint8_t>& data)
{
    std::vector<uint8_t> headerData;

    auto err = channel->WaitWrite(headerData, sizeof(AosProtobufHeader), timeout);
    if (!err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    auto header = reinterpret_cast<AosProtobufHeader*>(headerData.data());

    err = channel->WaitWrite(data, header->mDataSize, timeout);
    if (!err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    return aos::ErrorEnum::eNone;
}

/***********************************************************************************************************************
 * Public
 **********************************************************************************************************************/

aos::Error SendPBMessage(ChannelStub* channel, const void* message, size_t messageSize, const pb_msgdesc_t* fields)
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

    return SendMessageToChannel(channel, data);
}

aos::Error ReceivePBMessage(ChannelStub* channel, const std::chrono::duration<double>& timeout, void* message,
    size_t messageSize, const pb_msgdesc_t* fields)
{
    std::vector<uint8_t> data(messageSize);

    auto err = ReceiveMessageFromChannel(channel, timeout, data);
    if (!err.IsNone()) {
        return err;
    }

    if (message && fields) {
        auto stream = pb_istream_from_buffer(data.data(), data.size());

        auto status = pb_decode(&stream, fields, message);
        if (!status) {
            return aos::ErrorEnum::eRuntime;
        }
    }

    return aos::ErrorEnum::eNone;
}
