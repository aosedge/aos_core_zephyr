/*
 * Copyright (C) 2024 Renesas Electronics Corporation.
 * Copyright (C) 2024 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _UTILS_PBMESSAGES_HPP
#define _UTILS_PBMESSAGES_HPP

#include <pb.h>

#include "stubs/channelstub.hpp"

/**
 * Sends protobuf message.
 *
 * @param channel communication channel.
 * @param message protobuf message.
 * @param messageSize message size.
 * @param fields protobuf fields.
 * @return aos::Error
 */
aos::Error SendPBMessage(
    ChannelStub* channel, const void* message = nullptr, size_t messageSize = 0, const pb_msgdesc_t* fields = nullptr);

/**
 * Receives protobuf message.
 *
 * @param channel communication channel.
 * @param timeout timeout.
 * @param message protobuf message.
 * @param messageSize message size.
 * @param fields protobuf fields.
 * @return aos::Error
 */
aos::Error ReceivePBMessage(ChannelStub* channel, const std::chrono::duration<double>& timeout, void* message = nullptr,
    size_t messageSize = 0, const pb_msgdesc_t* fields = nullptr);

#endif
