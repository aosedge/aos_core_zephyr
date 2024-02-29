/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef COMMCHANNEL_HPP_
#define COMMCHANNEL_HPP_

#include <aos/common/tools/array.hpp>

/**
 * CommChannel interface.
 */
class CommChannelItf {
public:
    /**
     * Set TLS config.
     *
     * @param certType certificate type.
     * @return aos::Error.
     */
    virtual aos::Error SetTLSConfig(const aos::String& certType) = 0;

    /**
     * Connects to communication channel.
     *
     * @return aos::Error.
     */
    virtual aos::Error Connect() = 0;

    /**
     * Closes current connection.
     */
    virtual aos::Error Close() = 0;

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
    virtual ~CommChannelItf() { }
};

#endif
