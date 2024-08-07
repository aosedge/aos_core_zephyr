/*
 * Copyright (C) 2024 Renesas Electronics Corporation.
 * Copyright (C) 2024 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef PIPE_HPP_
#define PIPE_HPP_

#include <aos/common/tools/string.hpp>

#include "transport.hpp"

namespace aos::zephyr::communication {

class Transport : public TransportItf {
public:
    static constexpr auto cReadPath  = CONFIG_AOS_CHAN_TX_PATH;
    static constexpr auto cWritePath = CONFIG_AOS_CHAN_RX_PATH;

    /**
     * Initializes the pipe with file descriptors.
     *
     * @param readPipePath Path to the read pipe.
     * @param writePipePath Path to the write pipe.
     * @return aos::Error.
     */
    Error Init(const String& readPipePath, const String& writePipePath);

    /**
     * Opens the pipes.
     *
     * @return aos::Error.
     */
    Error Open() override;

    /**
     * Returns if the pipes are opened.
     *
     * @return bool.
     */
    bool IsOpened() const override { return mOpened; };

    /**
     * Closes the pipes.
     */
    Error Close() override;

    /**
     * Reads data from the pipe.
     *
     * @param data Buffer where data is placed.
     * @param size Specifies how many bytes to read.
     * @return int Number of bytes read.
     */
    int Read(void* data, size_t size) override;

    /**
     * Writes data to the pipe.
     *
     * @param data Data buffer.
     * @param size Specifies how many bytes to write.
     * @return int Number of bytes written.
     */
    int Write(const void* data, size_t size) override;

private:
    static constexpr auto cXSPathLen = 128;

    StaticString<cXSPathLen> mReadPipePath;
    StaticString<cXSPathLen> mWritePipePath;
    int                      mReadFd  = -1;
    int                      mWriteFd = -1;
    bool                     mOpened  = false;
};

} // namespace aos::zephyr::communication

#endif // PIPE_HPP_
