/*
 * Copyright (C) 2024 Renesas Electronics Corporation.
 * Copyright (C) 2024 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MESSAGEHANDLER_HPP_
#define MESSAGEHANDLER_HPP_

#include <aos/common/tools/array.hpp>

#include <pb_encode.h>
#include <vchanapi.h>

#include "channeltype.hpp"

/**
 * Max service length.
 */
constexpr auto cMaxServiceLen = 128;

/**
 * Max method length.
 */
constexpr auto cMaxMethodLen = 128;

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
     * @param messageError message error.
     * @return aos::Error.
     */
    virtual aos::Error SendMessage(Channel channel, AosVChanSource source, const aos::String& methodName,
        uint64_t requestID, const aos::Array<uint8_t> data, aos::Error messageError)
        = 0;

    /**
     * Destroys message sender.
     */
    virtual ~MessageSenderItf() = default;
};

/**
 * Protobuf service interface.
 */
class PBServiceItf {
public:
    /**
     * Returns service name.
     *
     * @return aos::String.
     */
    virtual const aos::String& GetServiceName() const = 0;

    /**
     * Registers method in service.
     *
     * @param methodName message method name.
     * @return aos::Error.
     */
    virtual aos::Error RegisterMethod(const aos::String& methodName) = 0;

    /**
     * Checks if method is registered.
     *
     * @param methodName
     * @return bool.
     */
    virtual bool IsMethodRegistered(const aos::String& methodName) const = 0;

    /**
     * Returns full method name if method is registered.
     *
     * @param methodName method name.
     * @param fullMethodName full method name.
     * @return aos::Error.
     */
    aos::Error GetFullMethodName(const aos::String& methodName, aos::String& fullMethodName) const
    {
        if (!IsMethodRegistered(methodName)) {
            return AOS_ERROR_WRAP(aos::ErrorEnum::eNotFound);
        }

        fullMethodName.Clear();
        fullMethodName.Append("/").Append(GetServiceName()).Append("/").Append(methodName);

        return aos::ErrorEnum::eNone;
    }

    /**
     * Destroys PB service.
     */
    virtual ~PBServiceItf() = default;
};

/**
 * Protobuf service implementation.
 */
template <size_t cMaxNumMethods>
class PBService : public PBServiceItf {
public:
    /**
     * Initializes service.
     *
     * @param name service name.
     * @return aos::Error.
     */
    aos::Error Init(const aos::String& serviceName)
    {
        mServiceName = serviceName;

        return aos::ErrorEnum::eNone;
    }

    /**
     * Returns service name.
     *
     * @return aos::String.
     */
    const aos::String& GetServiceName() const override { return mServiceName; }

    /**
     * Registers method in service.
     *
     * @param methodName message method name.
     * @return aos::Error.
     */
    aos::Error RegisterMethod(const aos::String& methodName) override
    {
        auto err = mMethods.PushBack(methodName);
        if (!err.IsNone()) {
            return AOS_ERROR_WRAP(err);
        }

        return aos::ErrorEnum::eNone;
    }

    /**
     * Checks if method is registered.
     *
     * @param methodName
     * @return bool.
     */
    bool IsMethodRegistered(const aos::String& methodName) const override
    {
        return mMethods.Find([&methodName](const auto& item) { return item == methodName; }).mError.IsNone();
    }

private:
    aos::StaticString<cMaxServiceLen>                                  mServiceName;
    aos::StaticArray<aos::StaticString<cMaxMethodLen>, cMaxNumMethods> mMethods;
};

/**
 * Message handler interface.
 */
class MessageHandlerItf {
public:
    /**
     * Processes received message.
     *
     * @param channel communication channel on which message is received.
     * @param fullMethodName protocol full method name.
     * @param requestID protocol request ID.
     * @param data raw message data.
     * @return aos::Error.
     */
    virtual aos::Error ReceiveMessage(
        Channel channel, const aos::String& fullMethodName, uint64_t requestID, const aos::Array<uint8_t>& data)
        = 0;

    /**
     * Returns send buffer.
     *
     * @return aos::Buffer.
     */
    virtual const aos::Buffer& GetSendBuffer() const = 0;

    /**
     * Returns receive buffer.
     *
     * @return aos::Buffer.
     */
    virtual const aos::Buffer& GetReceiveBuffer() const = 0;

    /**
     * Destroys message handler.
     */
    virtual ~MessageHandlerItf() = default;

protected:
    /**
     * Registers service to channel.
     *
     * @param service service to register.
     * @param channel channel.
     * @return aos::Error.
     */
    virtual aos::Error RegisterService(PBServiceItf& service, Channel channel) = 0;

    /**
     * Removes service from channel.
     *
     * @param service service to remove.
     * @param channel channel.
     * @return aos::Error.
     */
    virtual aos::Error RemoveService(PBServiceItf& service, Channel channel) = 0;

    /**
     * Sends protobuf message.
     *
     * @param channel channel to send message.
     * @param requestID request ID.
     * @param fields protobuf fields.
     * @param message protobuf message.
     * @param messageError message error.
     * @return aos::Error.
     */
    virtual aos::Error SendPBMessage(Channel channel, uint64_t requestID, const pb_msgdesc_t* fields,
        const void* message, aos::Error messageError = aos::ErrorEnum::eNone)
        = 0;

    /**
     * Processes received protobuf message.
     * @param channel channel on which message is received.
     * @param service protobuf service instance.
     * @param methodName method name.
     * @param requestID request ID.
     * @param data message data.
     * @return aos::Error.
     */
    virtual aos::Error ProcessMessage(Channel channel, PBServiceItf& service, const aos::String& methodName,
        uint64_t requestID, const aos::Array<uint8_t>& data)
        = 0;
};

/**
 * Message handler implementation.
 */
template <size_t cSendBufferSize, size_t cReceiveBufferSize, size_t cMaxNumServices = 1>
class MessageHandler : public MessageHandlerItf {
public:
    /**
     * Initializes message handler.
     *
     * @param source message source.
     * @param messageSender message sender instance.
     * @return aos::Error.
     */
    aos::Error Init(AosVChanSource source, MessageSenderItf& messageSender)
    {
        mSource        = source;
        mMessageSender = &messageSender;

        return aos::ErrorEnum::eNone;
    }

    /**
     * Processes received message.
     *
     * @param channel communication channel on which message is received.
     * @param fullMethodName protocol full method name.
     * @param requestID protocol request ID.
     * @param data raw message data.
     * @return aos::Error.
     */
    aos::Error ReceiveMessage(Channel channel, const aos::String& fullMethodName, uint64_t requestID,
        const aos::Array<uint8_t>& data) override
    {
        aos::StaticArray<aos::StaticString<aos::Max(cMaxServiceLen, cMaxMethodLen)>, 2> names;

        if (fullMethodName.Size() > 0) {
            auto err = (fullMethodName[0] != '/' ? fullMethodName : aos::String(&fullMethodName.CStr()[1]))
                           .Split(names, '/');
            if (!err.IsNone()) {
                return AOS_ERROR_WRAP(err);
            }
        }

        for (auto i = names.Size(); i < names.MaxSize(); i++) {
            names.PushBack("");
        }

        auto service = mServices[channel].Find([&names](auto item) { return item->GetServiceName() == names[0]; });

        if (!service.mError.IsNone() || !(*service.mValue)->IsMethodRegistered(names[1])) {
            return AOS_ERROR_WRAP(aos::ErrorEnum::eNotSupported);
        }

        return ProcessMessage(channel, **service.mValue, names[1], requestID, data);
    }

    /**
     * Returns send buffer.
     *
     * @return aos::Buffer.
     */
    const aos::Buffer& GetSendBuffer() const override { return mSendBuffer; };

    /**
     * Returns receive buffer.
     *
     * @return aos::Buffer.
     */
    const aos::Buffer& GetReceiveBuffer() const override { return mReceiveBuffer; };

protected:
    static constexpr auto cNodeID   = CONFIG_AOS_NODE_ID;
    static constexpr auto cNodeType = CONFIG_AOS_NODE_TYPE;

    aos::Error RegisterService(PBServiceItf& service, Channel channel) override
    {
        auto result = mServices[channel].Find(&service);
        if (result.mError.IsNone()) {
            return AOS_ERROR_WRAP(aos::ErrorEnum::eAlreadyExist);
        }

        if (result.mError == aos::ErrorEnum::eNotFound) {
            return mServices[channel].PushBack(&service);
        }

        return result.mError;
    }

    aos::Error RemoveService(PBServiceItf& service, Channel channel) override
    {
        return mServices[channel].Remove([&service](auto item) { return &service == item; }).mError;
    }

    aos::Error SendPBMessage(Channel channel, uint64_t requestID, const pb_msgdesc_t* fields, const void* message,
        aos::Error messageError = aos::ErrorEnum::eNone) override
    {
        auto outStream = pb_ostream_from_buffer(static_cast<pb_byte_t*>(GetSendBuffer().Get()), GetSendBuffer().Size());

        if (message && fields) {
            auto status = pb_encode(&outStream, fields, message);
            if (!status) {
                if (messageError.IsNone()) {
                    messageError = AOS_ERROR_WRAP(aos::ErrorEnum::eRuntime);
                }
            }
        }

        auto err = mMessageSender->SendMessage(channel, mSource, "", requestID,
            aos::Array<uint8_t>(static_cast<uint8_t*>(GetSendBuffer().Get()), outStream.bytes_written), messageError);
        if (!err.IsNone()) {
            return AOS_ERROR_WRAP(err);
        }

        return aos::ErrorEnum::eNone;
    }

private:
    AosVChanSource                                   mSource {};
    MessageSenderItf*                                mMessageSender {};
    aos::StaticBuffer<cSendBufferSize>               mSendBuffer;
    aos::StaticBuffer<cReceiveBufferSize>            mReceiveBuffer;
    aos::StaticArray<PBServiceItf*, cMaxNumServices> mServices[static_cast<size_t>(ChannelEnum::eNumChannels)];
};

#endif
