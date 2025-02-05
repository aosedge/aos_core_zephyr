/*
 * Copyright (C) 2024 Renesas Electronics Corporation.
 * Copyright (C) 2024 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TRANSPORT_HPP_
#define TRANSPORT_HPP_

#include <aos/common/tools/error.hpp>

namespace aos::zephyr::communication {

/**
 * Transport interface.
 */
class TransportItf {
public:
    /**
     * Opens transport.
     *
     * @return aos::Error.
     */
    virtual Error Open() = 0;

    /**
     * Closes transport.
     */
    virtual Error Close() = 0;

    /**
     * Returns if transport is opened.
     *
     * @return bool.
     */
    virtual bool IsOpened() const = 0;

    /**
     * Reads data from transport.
     *
     * @param data buffer where data is placed to.
     * @param size specifies how many bytes to read.
     * @return int num read bytes.
     */
    virtual int Read(void* data, size_t size) = 0;

    /**
     * Writes data to transport.
     *
     * @param data data buffer.
     * @param size specifies how many bytes to write.
     * @return int num written bytes.
     */
    virtual int Write(const void* data, size_t size) = 0;

    /**
     * Destructor.
     */
    virtual ~TransportItf() = default;
};

} // namespace aos::zephyr::communication

#endif
