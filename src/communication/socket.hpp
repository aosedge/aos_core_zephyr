/*
 * Copyright (C) 2024 Renesas Electronics Corporation.
 * Copyright (C) 2024 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SOCKET_HPP_
#define SOCKET_HPP_

#include <aos/common/tools/string.hpp>

#include "transport.hpp"

namespace aos::zephyr::communication {

class Socket : public TransportItf {
public:
    constexpr static auto cServerAddress = CONFIG_AOS_SOCKET_SERVER_ADDRESS;
    constexpr static auto cServerPort    = CONFIG_AOS_SOCKET_SERVER_PORT;

    /**
     * Initializes the socket.
     *
     * @param serverAddress Server address.
     * @param serverPort Server port.
     * @return Error.
     */
    Error Init(const String& serverAddress, int serverPort);

    /**
     * Opens the socket.
     *
     * @return Error.
     */
    Error Open() override;

    /**
     * Returns if the socket is opened.
     *
     * @return bool.
     */
    bool IsOpened() const override { return mOpened; };

    /**
     * Closes the pipes.
     */
    Error Close() override;

    /**
     * Reads data from the socket.
     *
     * @param data Buffer where data is placed.
     * @param size Specifies how many bytes to read.
     * @return int Number of bytes read.
     */
    int Read(void* data, size_t size) override;

    /**
     * Writes data to the socket.
     *
     * @param data Data buffer.
     * @param size Specifies how many bytes to write.
     * @return int Number of bytes written.
     */
    int Write(const void* data, size_t size) override;

private:
    static constexpr auto cIPAddrLen = 16;

    int ReadFromSocket(int fd, void* data, size_t size);
    int WriteToSocket(int fd, const void* data, size_t size);

    StaticString<cIPAddrLen> mServerAddress;
    int                      mServerPort {-1};
    int                      mSocketFd {-1};
    bool                     mOpened {};
};

} // namespace aos::zephyr::communication

#endif // SOCKET_HPP_
