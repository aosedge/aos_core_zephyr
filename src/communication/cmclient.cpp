/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <aos/common/tools/memory.hpp>

#include <pb_decode.h>
#include <pb_encode.h>

#include "utils/pbconvert.hpp"

#include "cmclient.hpp"
#include "log.hpp"

/***********************************************************************************************************************
 * Public
 **********************************************************************************************************************/

aos::Error CMClient::Init(MessageSenderItf& messageSender)
{
    LOG_DBG() << "Initialize CM client";

    mMessageSender = &messageSender;

    return aos::ErrorEnum::eNone;
}

aos::Error CMClient::ProcessMessage(const aos::String& methodName, uint64_t requestID, const aos::Array<uint8_t>& data)
{
    (void)methodName;

    auto message = aos::MakeUnique<servicemanager_v3_SMIncomingMessages>(&mAllocator);
    auto stream  = pb_istream_from_buffer(data.Get(), data.Size());

    auto status = pb_decode(&stream, servicemanager_v3_SMIncomingMessages_fields, message.Get());
    if (!status) {
        LOG_ERR() << "Can't decode incoming message: " << PB_GET_ERROR(&stream);
        return AOS_ERROR_WRAP(aos::ErrorEnum::eRuntime);
    }

    aos::Error err;

    switch (message->which_SMIncomingMessage) {
    case servicemanager_v3_SMIncomingMessages_get_unit_config_status_tag:
        break;

    case servicemanager_v3_SMIncomingMessages_check_unit_config_tag:
        break;

    case servicemanager_v3_SMIncomingMessages_set_unit_config_tag:
        break;

    case servicemanager_v3_SMIncomingMessages_run_instances_tag:
        break;

    case servicemanager_v3_SMIncomingMessages_image_content_info_tag:
        break;

    case servicemanager_v3_SMIncomingMessages_image_content_tag:
        break;

    default:
        LOG_WRN() << "Receive unsupported message tag: " << message->which_SMIncomingMessage;
        break;
    }

    return err;
}

/***********************************************************************************************************************
 * Private
 **********************************************************************************************************************/

aos::Error CMClient::SendOutgoingMessage(const servicemanager_v3_SMOutgoingMessages& message, aos::Error messageError)
{
    auto outStream = pb_ostream_from_buffer(static_cast<pb_byte_t*>(mSendBuffer.Get()), mSendBuffer.Size());

    auto status = pb_encode(&outStream, servicemanager_v3_SMOutgoingMessages_fields, &message);
    if (!status) {
        LOG_ERR() << "Can't encode outgoing message: " << PB_GET_ERROR(&outStream);

        if (messageError.IsNone()) {
            messageError = AOS_ERROR_WRAP(aos::ErrorEnum::eRuntime);
        }
    }

    return mMessageSender->SendMessage(ChannelEnum::eSecure, AOS_VCHAN_SM, "", 0,
        aos::Array<uint8_t>(static_cast<uint8_t*>(mSendBuffer.Get()), outStream.bytes_written), messageError);
}
