/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <chrono>
#include <condition_variable>

#include <zephyr/tc_util.h>
#include <zephyr/ztest.h>

#include <pb_decode.h>
#include <pb_encode.h>

#include <aos/common/tools/log.hpp>

#include "communication/communication.hpp"
#include "utils/checksum.hpp"
#include "utils/pbconvert.hpp"

#include "mocks/commchannelmock.hpp"
#include "mocks/connectionsubscribermock.hpp"
#include "mocks/resourcemanagermock.hpp"

/***********************************************************************************************************************
 * Types
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * Static
 **********************************************************************************************************************/

static constexpr auto cWaitTimeout = std::chrono::seconds {1};

/***********************************************************************************************************************
 * Vars
 **********************************************************************************************************************/

static CommChannelMock sOpenChannel;
static CommChannelMock sSecureChannel;
ResourceManagerMock    sResourceManager;
static Communication   sCommunication;

/***********************************************************************************************************************
 * Private
 **********************************************************************************************************************/

static aos::Error SendMessageToChannel(CommChannelMock& channel, uint32_t source, const std::string& methodName,
    uint64_t requestID, const std::vector<uint8_t>& data, aos::Error messageError = aos::ErrorEnum::eNone)
{
    auto checksum = CalculateSha256(aos::Array<uint8_t>(data.data(), data.size()));
    if (!checksum.mError.IsNone()) {
        return AOS_ERROR_WRAP(checksum.mError);
    }

    auto header = VChanMessageHeader {source, static_cast<uint32_t>(data.size()), requestID, messageError.Errno(),
        static_cast<int32_t>(messageError.Value()), ""};

    aos::Array<uint8_t>(header.mSha256, aos::cSHA256Size) = checksum.mValue;

    auto headerPtr = reinterpret_cast<uint8_t*>(&header);

    channel.SendRead(std::vector<uint8_t>(headerPtr, headerPtr + sizeof(VChanMessageHeader)));
    channel.SendRead(data);

    return aos::ErrorEnum::eNone;
}

static aos::Error ReceiveMessageFromChannel(CommChannelMock& channel, uint32_t source, const std::string& methodName,
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

    auto checksum = CalculateSha256(aos::Array<uint8_t>(data.data(), data.size()));
    if (!checksum.mError.IsNone()) {
        return AOS_ERROR_WRAP(checksum.mError);
    }

    if (checksum.mValue != aos::Array<uint8_t>(header->mSha256, aos::cSHA256Size)) {
        return AOS_ERROR_WRAP(aos::ErrorEnum::eInvalidChecksum);
    }

    return aos::ErrorEnum::eNone;
}

static aos::Error SendCMIncomingMessage(
    CommChannelMock& channel, uint32_t source, const servicemanager_v3_SMIncomingMessages& message)
{
    std::vector<uint8_t> data(servicemanager_v3_SMIncomingMessages_size);

    auto stream = pb_ostream_from_buffer(data.data(), data.size());

    auto status = pb_encode(&stream, servicemanager_v3_SMIncomingMessages_fields, &message);
    if (!status) {
        return aos::ErrorEnum::eFailed;
    }

    data.resize(stream.bytes_written);

    return SendMessageToChannel(channel, source, "", 0, data);
}

static aos::Error ReceiveCMOutgoingMessage(
    CommChannelMock& channel, uint32_t source, servicemanager_v3_SMOutgoingMessages& message)
{
    std::vector<uint8_t> data(servicemanager_v3_SMOutgoingMessages_size);

    auto err = ReceiveMessageFromChannel(channel, source, "", 0, data);
    if (!err.IsNone()) {
        return err;
    }

    auto stream = pb_istream_from_buffer(data.data(), data.size());

    auto status = pb_decode(&stream, servicemanager_v3_SMOutgoingMessages_fields, &message);
    if (!status) {
        return AOS_ERROR_WRAP(aos::ErrorEnum::eRuntime);
    }

    return aos::ErrorEnum::eNone;
}

/***********************************************************************************************************************
 * Setup
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

ZTEST_SUITE(
    communication, NULL,
    []() -> void* {
        aos::Log::SetCallback(TestLogCallback);

        auto err = sCommunication.Init(sOpenChannel, sSecureChannel, sResourceManager);
        zassert_true(err.IsNone(), "Can't initialize communication: %s", err.Message());

        return nullptr;
    },
    [](void*) {}, NULL, NULL);

/***********************************************************************************************************************
 * Tests
 **********************************************************************************************************************/

ZTEST_F(communication, test_Connection)
{
    ConnectionSubscriberMock subscriber;

    sCommunication.Subscribes(subscriber);

    sSecureChannel.SendRead(std::vector<uint8_t>(), aos::ErrorEnum::eFailed);

    auto err = subscriber.WaitDisconnect(cWaitTimeout);
    zassert_true(err.IsNone(), "Wait connection error: %s", err.Message());

    err = subscriber.WaitConnect(cWaitTimeout);
    zassert_true(err.IsNone(), "Wait connection error: %s", err.Message());

    sCommunication.Unsubscribes(subscriber);
}

ZTEST_F(communication, test_GetUnitConfigStatus)
{
    auto version    = "1.1.1";
    auto unitConfig = "unitConfig";

    sResourceManager.UpdateUnitConfig(version, unitConfig);
    sResourceManager.SetError(aos::ErrorEnum::eFailed);

    servicemanager_v3_SMIncomingMessages incomingMessage servicemanager_v3_SMIncomingMessages_init_zero;

    incomingMessage.which_SMIncomingMessage = servicemanager_v3_SMIncomingMessages_get_unit_config_status_tag;

    auto err = SendCMIncomingMessage(sSecureChannel, AOS_VCHAN_SM, incomingMessage);
    zassert_true(err.IsNone(), "Error sending message: %s", err.Message());

    servicemanager_v3_SMOutgoingMessages outgoingMessage servicemanager_v3_SMOutgoingMessages_init_zero;

    err = ReceiveCMOutgoingMessage(sSecureChannel, AOS_VCHAN_SM, outgoingMessage);
    zassert_true(err.IsNone(), "Error receiving message: %s", err.Message());

    zassert_equal(outgoingMessage.which_SMOutgoingMessage, servicemanager_v3_SMOutgoingMessages_unit_config_status_tag);
    zassert_equal(strcmp(outgoingMessage.SMOutgoingMessage.unit_config_status.vendor_version, version), 0);
    zassert_equal(strcmp(outgoingMessage.SMOutgoingMessage.unit_config_status.error,
                      aos::Error(aos::ErrorEnum::eFailed).Message()),
        0);

    sResourceManager.SetError(aos::ErrorEnum::eNone);
}

ZTEST_F(communication, test_CheckUnitConfig)
{
    auto                                                 version    = "1.2.0";
    auto                                                 unitConfig = "unitConfig";
    servicemanager_v3_SMIncomingMessages incomingMessage servicemanager_v3_SMIncomingMessages_init_zero;

    incomingMessage.which_SMIncomingMessage = servicemanager_v3_SMIncomingMessages_check_unit_config_tag;
    strcpy(incomingMessage.SMIncomingMessage.check_unit_config.vendor_version, version);
    strcpy(incomingMessage.SMIncomingMessage.check_unit_config.unit_config, unitConfig);

    auto err = SendCMIncomingMessage(sSecureChannel, AOS_VCHAN_SM, incomingMessage);
    zassert_true(err.IsNone(), "Error sending message: %s", err.Message());

    servicemanager_v3_SMOutgoingMessages outgoingMessage servicemanager_v3_SMOutgoingMessages_init_zero;

    err = ReceiveCMOutgoingMessage(sSecureChannel, AOS_VCHAN_SM, outgoingMessage);
    zassert_true(err.IsNone(), "Error receiving message: %s", err.Message());

    zassert_equal(outgoingMessage.which_SMOutgoingMessage, servicemanager_v3_SMOutgoingMessages_unit_config_status_tag);
    zassert_equal(strcmp(outgoingMessage.SMOutgoingMessage.unit_config_status.vendor_version, version), 0);
    zassert_equal(strlen(outgoingMessage.SMOutgoingMessage.unit_config_status.error), 0);
}

ZTEST_F(communication, test_SetUnitConfig)
{
    auto                                                 version    = "1.2.0";
    auto                                                 unitConfig = "unitConfig";
    servicemanager_v3_SMIncomingMessages incomingMessage servicemanager_v3_SMIncomingMessages_init_zero;

    incomingMessage.which_SMIncomingMessage = servicemanager_v3_SMIncomingMessages_set_unit_config_tag;
    strcpy(incomingMessage.SMIncomingMessage.set_unit_config.vendor_version, version);
    strcpy(incomingMessage.SMIncomingMessage.set_unit_config.unit_config, unitConfig);

    auto err = SendCMIncomingMessage(sSecureChannel, AOS_VCHAN_SM, incomingMessage);
    zassert_true(err.IsNone(), "Error sending message: %s", err.Message());

    servicemanager_v3_SMOutgoingMessages outgoingMessage servicemanager_v3_SMOutgoingMessages_init_zero;

    err = ReceiveCMOutgoingMessage(sSecureChannel, AOS_VCHAN_SM, outgoingMessage);
    zassert_true(err.IsNone(), "Error receiving message: %s", err.Message());

    zassert_equal(outgoingMessage.which_SMOutgoingMessage, servicemanager_v3_SMOutgoingMessages_unit_config_status_tag);
    zassert_equal(strcmp(outgoingMessage.SMOutgoingMessage.unit_config_status.vendor_version, version), 0);
    zassert_equal(strlen(outgoingMessage.SMOutgoingMessage.unit_config_status.error), 0);
    zassert_equal(sResourceManager.GetVersion(), version);
    zassert_equal(sResourceManager.GetUnitConfig(), unitConfig);
}
