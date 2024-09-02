/*
 * Copyright (C) 2024 Renesas Electronics Corporation.
 * Copyright (C) 2024 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef PBHANDLER_HPP_
#define PBHANDLER_HPP_

#include <pb_encode.h>

#include <aos/common/tools/error.hpp>
#include <aos/common/tools/thread.hpp>
#include <aosprotocol.h>

#include "channel.hpp"

namespace aos::zephyr::communication {

/**
 * Protobuf handler.
 */
template <size_t cReceiveBufferSize, size_t cSendBufferSize>
class PBHandler {
public:
    /**
     * Constructor.
     */
    PBHandler() = default;

    /**
     * Initializes protobuf handler.
     *
     * @param channel communication channel.
     * @return Error
     */
    Error Init(const String& name, ChannelItf& channel);

    /**
     *  Starts protobuf handler.
     *
     * @return Error.
     */
    Error Start();

    /**
     * Stops protobuf handler.
     *
     * @return Error.
     */
    Error Stop();

    /**
     * Returns true if handler is started.
     *
     * @return bool.
     */
    bool IsStarted() const
    {
        LockGuard lock {mMutex};

        return mStarted;
    }

    /**
     * Destructor.
     */
    virtual ~PBHandler();

protected:
    /**
     * Connect notification.
     */
    virtual void OnConnect() = 0;

    /**
     * Disconnect notification.
     */
    virtual void OnDisconnect() = 0;

    /**
     * Sends protobuf message.
     *
     * @param message message to send.
     * @param fields message fields.
     * @return Error.
     */
    Error SendMessage(const void* message, const pb_msgdesc_t* fields);

    /**
     * Receives protobuf message.
     *
     * @param data received data.
     * @return Error.
     */
    virtual Error ReceiveMessage(const Array<uint8_t>& data) = 0;

private:
    static constexpr auto cThreadStackSize = CONFIG_AOS_PBHANDLER_THREAD_STACK_SIZE;

    void Run();

    StaticString<64>                                               mName;
    ChannelItf*                                                    mChannel;
    mutable Mutex                                                  mMutex;
    ConditionalVariable                                            mCondVar;
    Thread<cDefaultFunctionMaxSize, cThreadStackSize>              mThread;
    bool                                                           mStarted = false;
    aos::StaticBuffer<cSendBufferSize + sizeof(AosProtobufHeader)> mSendBuffer;
    aos::StaticBuffer<cReceiveBufferSize>                          mReceiveBuffer;
};

} // namespace aos::zephyr::communication

#endif
