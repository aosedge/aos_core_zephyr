/*
 * Copyright (C) 2024 Renesas Electronics Corporation.
 * Copyright (C) 2024 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <pb.h>

#include "mocks/commchannelmock.hpp"

/***********************************************************************************************************************
 * Consts
 **********************************************************************************************************************/

constexpr auto cWaitTimeout = std::chrono::seconds {5};

/***********************************************************************************************************************
 * Public
 **********************************************************************************************************************/

void TestLogCallback(aos::LogModule module, aos::LogLevel level, const aos::String& message);

aos::Error SendMessageToChannel(CommChannelMock& channel, uint32_t source, const std::string& methodName,
    uint64_t requestID, const std::vector<uint8_t>& data, aos::Error messageError = aos::ErrorEnum::eNone);

aos::Error ReceiveMessageFromChannel(CommChannelMock& channel, uint32_t source, const std::string& methodName,
    uint64_t requestID, std::vector<uint8_t>& data);

aos::Error SendPBMessage(CommChannelMock& channel, uint32_t source, const std::string& methodName, uint64_t requestID,
    const pb_msgdesc_t* fields = nullptr, const void* message = nullptr, size_t messageSize = 0);

aos::Error ReceivePBMessage(CommChannelMock& channel, uint32_t source, const std::string& methodName,
    uint64_t requestID, const pb_msgdesc_t* fields = nullptr, void* message = nullptr, size_t messageSize = 0);
