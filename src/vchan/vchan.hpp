/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef VCHAN_HPP_
#define VCHAN_HPP_

#include <aos/common/tools/error.hpp>
#include <aos/common/tools/string.hpp>

#include <vch.h>

class Vchan {
public:
    /**
     * Creates Vchan instance.
     */
    Vchan() = default;

    /**
     * Destructor.
     */
    ~Vchan();

    /**
     * Initializes Vchan instance.
     *
     * @return Error.
     */
    virtual aos::Error Init();

    /**
     * Connects to Vchan.
     *
     * @param domdID domd ID.
     * @param vchanPath Vchan path.
     * @return Error.
     */
    virtual aos::Error Connect(domid_t domdID, const aos::String& vchanPath);

    /**
     * Disconnects from Vchan.
     *
     * @return Error.
     */
    virtual void Disconnect();

    /**
     * Reads data from Vchan.
     *
     * @param buffer buffer to read.
     * @param size buffer size.
     * @return Error.
     */
    virtual aos::Error Read(uint8_t* buffer, size_t size);

    /**
     * Writes data to Vchan.
     *
     * @param buffer buffer to write.
     * @param size buffer size.
     * @return Error.
     */
    virtual aos::Error Write(const uint8_t* buffer, size_t size);

    /**
     * Sets blocking mode.
     *
     * @param blocking blocking mode.
     */
    void SetBlocking(bool blocking);

private:
    vch_handle mVchanHandle {};
};

#endif
