/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MESSAGESENDER_HPP_
#define MESSAGESENDER_HPP_

#include <aos/common/tools/array.hpp>

#include <vchanapi.h>

#include "channeltype.hpp"

/**
 * Interface to send messages.
 */
class MessageSenderItf {
public:
    /**
     * Sends raw message.
     *
     * @param channel channel to send message.
     * @param source message source.
     * @param methodName message method name.
     * @param requestID request ID.
     * @param data message data.
     * @param err message error.
     * @return aos::Error
     */
    virtual aos::Error SendMessage(Channel channel, AosVChanSource source, const aos::String& methodName,
        uint64_t requestID, const aos::Array<uint8_t> data, aos::Error messageError)
        = 0;
};

#endif
