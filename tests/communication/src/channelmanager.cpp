/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <chrono>
#include <mutex>
#include <thread>

#include <zephyr/kernel.h>
#include <zephyr/tc_util.h>
#include <zephyr/ztest.h>

#include <aos/common/tools/log.hpp>

#include "communication/channelmanager.hpp"
#include "utils/checksum.hpp"
#include "utils/log.hpp"

#include "stubs/transportstub.hpp"

/***********************************************************************************************************************
 * Static
 **********************************************************************************************************************/

static aos::RetWithError<AosProtocolHeader> PrepareHeader(uint32_t port, const aos::Array<uint8_t>& data)
{
    AosProtocolHeader header;
    header.mPort     = port;
    header.mDataSize = data.Size();

    auto ret = aos::zephyr::utils::CalculateSha256(data);
    if (!ret.mError.IsNone()) {
        return {header, AOS_ERROR_WRAP(ret.mError)};
    }

    aos::Array<uint8_t>(reinterpret_cast<uint8_t*>(header.mCheckSum), aos::cSHA256Size) = ret.mValue;

    return header;
}

/***********************************************************************************************************************
 * Setup
 **********************************************************************************************************************/

ZTEST_SUITE(channelmanager, nullptr, nullptr, nullptr, nullptr, nullptr);

/***********************************************************************************************************************
 * Tests
 **********************************************************************************************************************/

ZTEST(channelmanager, test_message_exchange)
{
    aos::Log::SetCallback(TestLogCallback);

    aos::zephyr::communication::Pipe           pipe1;
    aos::zephyr::communication::Pipe           pipe2;
    aos::zephyr::communication::TransportStub  transport(pipe1, pipe2);
    aos::zephyr::communication::ChannelManager channelManager;

    auto err = channelManager.Init(transport);
    zassert_true(err.IsNone(), "Channel manager initialization failed");

    err = channelManager.Start();
    zassert_true(err.IsNone(), "Channel manager start failed");

    auto ret = channelManager.CreateChannel(8080);
    zassert_true(ret.mError.IsNone(), "Channel creation failed", ret.mError.Message());
    zassert_true(ret.mValue != nullptr, "Channel creation failed");

    const char* msg = "Test1";
    ret.mValue->Write(msg, strlen(msg));

    AosProtocolHeader header;
    int               bytesRead = pipe2.Read(reinterpret_cast<uint8_t*>(&header), sizeof(AosProtocolHeader));
    zassert_true(bytesRead == sizeof(AosProtocolHeader), "Failed to read header");
    zassert_equal(header.mPort, 8080, "Port mismatch");

    char buffer[100] {};
    bytesRead         = pipe2.Read(reinterpret_cast<uint8_t*>(buffer), header.mDataSize);
    buffer[bytesRead] = '\0';
    zassert_equal(strcmp(buffer, msg), 0, "Message read from transport does not match");

    char response[] = "Test2!";
    auto headerRet  = PrepareHeader(8080, aos::Array<uint8_t>(reinterpret_cast<uint8_t*>(response), strlen(response)));
    zassert_true(headerRet.mError.IsNone(), "Failed to prepare header");

    pipe1.Write(reinterpret_cast<uint8_t*>(&headerRet.mValue), sizeof(AosProtocolHeader));
    pipe1.Write(reinterpret_cast<uint8_t*>(response), strlen(response));

    memset(buffer, 0, sizeof(buffer));

    bytesRead         = ret.mValue->Read(buffer, strlen(response));
    buffer[bytesRead] = '\0';
    zassert_equal(strcmp(buffer, response), 0, "Message read from transport does not match");

    pipe1.Close();
    pipe2.Close();

    channelManager.Close();
}

ZTEST(channelmanager, test_read_message)
{
    aos::Log::SetCallback(TestLogCallback);

    aos::zephyr::communication::Pipe           pipe1;
    aos::zephyr::communication::Pipe           pipe2;
    aos::zephyr::communication::TransportStub  transport(pipe1, pipe2);
    aos::zephyr::communication::ChannelManager channelManager;

    auto err = channelManager.Init(transport);
    zassert_true(err.IsNone(), "Channel manager initialization failed");

    err = channelManager.Start();
    zassert_true(err.IsNone(), "Channel manager start failed");

    auto ret = channelManager.CreateChannel(8080);
    zassert_true(ret.mError.IsNone(), "Channel creation failed", ret.mError.Message());
    zassert_true(ret.mValue != nullptr, "Channel creation failed");

    char response[] = "Test!";
    auto headerRet  = PrepareHeader(8080, aos::Array<uint8_t>(reinterpret_cast<uint8_t*>(response), strlen(response)));
    zassert_true(headerRet.mError.IsNone(), "Failed to prepare header");

    std::thread tRead([&]() {
        char buffer[100] {};
        auto bytesRead = ret.mValue->Read(buffer, strlen(response));

        buffer[bytesRead] = '\0';
        zassert_equal(strcmp(buffer, response), 0, "Message read from transport does not match");
    });

    pipe1.Write(reinterpret_cast<uint8_t*>(&headerRet.mValue), sizeof(AosProtocolHeader));
    pipe1.Write(reinterpret_cast<uint8_t*>(response), strlen(response));

    tRead.join();

    pipe1.Close();
    pipe2.Close();

    channelManager.Close();
}

ZTEST(channelmanager, test_read_message_in_chunks)
{
    aos::Log::SetCallback(TestLogCallback);

    aos::zephyr::communication::Pipe           pipe1;
    aos::zephyr::communication::Pipe           pipe2;
    aos::zephyr::communication::TransportStub  transport(pipe1, pipe2);
    aos::zephyr::communication::ChannelManager channelManager;

    auto err = channelManager.Init(transport);
    zassert_true(err.IsNone(), "Channel manager initialization failed");

    err = channelManager.Start();
    zassert_true(err.IsNone(), "Channel manager start failed");

    auto ret = channelManager.CreateChannel(8080);
    zassert_true(ret.mError.IsNone(), "Channel creation failed", ret.mError.Message());
    zassert_true(ret.mValue != nullptr, "Channel creation failed");

    char response[] = "Test";
    auto headerRet  = PrepareHeader(8080, aos::Array<uint8_t>(reinterpret_cast<uint8_t*>(response), strlen(response)));
    zassert_true(headerRet.mError.IsNone(), "Failed to prepare header");

    std::thread tRead([&]() {
        char resultBuffer[100] {};
        char buffer[100] {};
        auto bytesRead = ret.mValue->Read(buffer, strlen(response) / 2);
        memcpy(resultBuffer, buffer, bytesRead);

        memset(buffer, 0, sizeof(buffer));

        bytesRead = ret.mValue->Read(buffer, strlen(response) / 2);
        memcpy(resultBuffer + strlen(response) / 2, buffer, bytesRead);

        resultBuffer[strlen(response)] = '\0';

        zassert_equal(strcmp(resultBuffer, response), 0, "Message read from transport does not match");
    });

    pipe1.Write(reinterpret_cast<uint8_t*>(&headerRet.mValue), sizeof(AosProtocolHeader));
    pipe1.Write(reinterpret_cast<uint8_t*>(response), strlen(response));

    tRead.join();

    pipe1.Close();
    pipe2.Close();

    channelManager.Close();
}
