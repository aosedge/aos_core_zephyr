/*
 * Copyright (C) 2024 Renesas Electronics Corporation.
 * Copyright (C) 2024 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CHANNEL_HPP_
#define CHANNEL_HPP_

#include <aos/common/tools/error.hpp>
#include <aos/common/tools/thread.hpp>
#include <aosprotocol.h>

#include "transport.hpp"

namespace aos::zephyr::communication {

/**
 * Channel interface.
 */
class ChannelItf {
public:
    /**
     * Connects to communication channel.
     *
     * @return aos::Error.
     */
    virtual Error Connect() = 0;

    /**
     * Closes current connection.
     */
    virtual Error Close() = 0;

    /**
     * Returns if channel is connected.
     *
     * @return bool.
     */
    virtual bool IsConnected() const = 0;

    /**
     * Reads data from channel to array.
     *
     * @param data buffer where data is placed to.
     * @param size specifies how many bytes to read.
     * @return int num read bytes.
     */
    virtual int Read(void* data, size_t size) = 0;

    /**
     * Writes data from array to channel.
     *
     * @param data data buffer.
     * @param size specifies how many bytes to write.
     * @return int num written bytes.
     */
    virtual int Write(const void* data, size_t size) = 0;

    /**
     * Destructor.
     */
    virtual ~ChannelItf() = default;
};

/**
 * Communication interface.
 */
class CommunicationItf {
public:
    /**
     * Connects to communication channel.
     *
     * @return aos::Error.
     */
    virtual Error Connect() = 0;

    /**
     * Closes current connection.
     *
     * @return aos::Error.
     */
    virtual bool IsConnected() const = 0;

    /**
     * Reads data from channel to array.
     *
     * @param data buffer where data is placed to.
     * @param size specifies how many bytes to read.
     * @return int num read bytes.
     */
    virtual int Write(uint32_t port, const void* data, size_t size) = 0;
};

/**
 * Channel class.
 */
class Channel : public ChannelItf {
public:
    /**
     * Constructor.
     *
     * @param communication communication interface.
     * @param port port number.
     */
    Channel(CommunicationItf* communication, int port);

    /**
     * Connects to communication channel.
     *
     * @return aos::Error.
     */
    Error Connect() override;

    /**
     * Closes current connection.
     *
     * @return aos::Error.
     */
    Error Close() override;

    /**
     * Reads data from channel to buffer.
     *
     * @param data buffer where data is placed to.
     * @param size specifies how many bytes to read.
     * @return int num read bytes.
     */
    int Read(void* buffer, size_t size) override;

    /**
     * Writes data from buffer to channel.
     *
     * @param data data buffer.
     * @param size specifies how many bytes to write.
     * @return int num written bytes.
     */
    int Write(const void* buffer, size_t size) override;

    /**
     * Returns if channel is connected.
     *
     * @return bool.
     */
    bool IsConnected() const override;

    /**
     * Waits for read.
     *
     * @param buffer buffer where data is placed to.
     * @param size specifies how many bytes to read.
     * @return aos::Error.
     */
    Error WaitRead(void** buffer, size_t* size);

    /**
     * Notifies that read is done.
     *
     * @param size specifies how many bytes were read.
     * @return aos::Error.
     */
    Error ReadDone(size_t size);

private:
    static constexpr auto cWaitReadPeriod = 3 * Time::cSeconds;

    CommunicationItf*   mCommunication {};
    int                 mPort {};
    bool                mReadReady {};
    bool                mClose {};
    uint8_t*            mBuffer {};
    size_t              mBufferLen {};
    mutable Mutex       mMutex;
    ConditionalVariable mCondVar;
};

} // namespace aos::zephyr::communication

#endif
