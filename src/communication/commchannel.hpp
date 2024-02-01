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
     * Set the secure channel.
     *
     * @param certType Certificate type.
     * @return aos::Error.
     */
    virtual aos::Error SetSecure(const aos::String& certType) = 0;

    /**
     * Connects to communication channel.
     *
     * @param secure specifies if secure connection is required.
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
     * @param data array where data is placed to.
     * @param size specifies how many bytes to read.
     * @return aos::Error.
     */
    virtual aos::Error Read(aos::Array<uint8_t>& data, size_t size) = 0;

    /**
     * Writes data from array to channel.
     *
     * @param data array where data is placed to.
     * @return aos::Error.
     */
    virtual aos::Error Write(const aos::Array<uint8_t>& data) = 0;

    /**
     * Destructor.
     */
    virtual ~CommChannelItf() { }
};

#endif
