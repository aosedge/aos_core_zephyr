/*
 * Copyright (C) 2024 Renesas Electronics Corporation.
 * Copyright (C) 2024 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <pb.h>

#include "stubs/commchannelstub.hpp"

/***********************************************************************************************************************
 * Consts
 **********************************************************************************************************************/

constexpr auto cWaitTimeout = std::chrono::seconds {5};

/***********************************************************************************************************************
 * Public
 **********************************************************************************************************************/

void TestLogCallback(const char* module, aos::LogLevel level, const aos::String& message);

aos::Error SendMessageToChannel(CommChannelStub& channel, uint32_t source, const std::string& methodName,
    uint64_t requestID, const std::vector<uint8_t>& data, aos::Error messageError = aos::ErrorEnum::eNone);

aos::Error ReceiveMessageFromChannel(CommChannelStub& channel, uint32_t source, const std::string& methodName,
    uint64_t requestID, std::vector<uint8_t>& data);

aos::Error SendPBMessage(CommChannelStub& channel, uint32_t source, const std::string& methodName, uint64_t requestID,
    const pb_msgdesc_t* fields = nullptr, const void* message = nullptr, size_t messageSize = 0);

aos::Error ReceivePBMessage(CommChannelStub& channel, uint32_t source, const std::string& methodName,
    uint64_t requestID, const pb_msgdesc_t* fields = nullptr, void* message = nullptr, size_t messageSize = 0);
