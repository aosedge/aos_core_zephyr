/*
 * Copyright (C) 2024 Renesas Electronics Corporation.
 * Copyright (C) 2024 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include "log.hpp"
#include "socket.hpp"

namespace aos::zephyr::communication {

/***********************************************************************************************************************
 * Public
 **********************************************************************************************************************/

Error Socket::Init(const String& serverAddress, int serverPort)
{
    mServerAddress = serverAddress;
    mServerPort    = serverPort;
    mSocketFd      = -1;

    return ErrorEnum::eNone;
}

Error Socket::Open()
{
    LockGuard lock {mMutex};

    LOG_INF() << "Connecting socket to: address=" << mServerAddress << ", port=" << mServerPort;

    mSocketFd = socket(AF_INET, SOCK_STREAM, 0);
    if (mSocketFd == -1) {
        return Error(ErrorEnum::eRuntime, "failed to create socket");
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(mServerPort);

    if (inet_pton(AF_INET, mServerAddress.CStr(), &addr.sin_addr) <= 0) {
        close(mSocketFd);
        mSocketFd = -1;
        return Error(ErrorEnum::eRuntime, "invalid server address");
    }

    if (connect(mSocketFd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        LOG_ERR() << "Failed to connect to server: address=" << mServerAddress.CStr() << ", port=" << mServerPort;

        close(mSocketFd);
        mSocketFd = -1;

        return Error(ErrorEnum::eRuntime, "failed to connect to server");
    }

    LOG_INF() << "Connected to server: address=" << mServerAddress.CStr() << ", port=" << mServerPort;

    mOpened = true;

    return ErrorEnum::eNone;
}

Error Socket::Close()
{
    LockGuard lock {mMutex};

    if (mSocketFd != -1) {
        close(mSocketFd);
        mSocketFd = -1;
    }

    mOpened = false;

    return ErrorEnum::eNone;
}

bool Socket::IsOpened() const
{
    LockGuard lock {mMutex};

    return mOpened;
}

int Socket::Read(void* data, size_t size)
{
    LOG_DBG() << "Read from server: size=" << size;

    return ReadFromSocket(mSocketFd, data, size);
}

int Socket::Write(const void* data, size_t size)
{
    LOG_DBG() << "Write to server: size=" << size;

    return WriteToSocket(mSocketFd, data, size);
}

/***********************************************************************************************************************
 * Private
 **********************************************************************************************************************/

int Socket::ReadFromSocket(int fd, void* data, size_t size)
{
    ssize_t readBytes = 0;

    while (readBytes < static_cast<ssize_t>(size)) {
        ssize_t len = recv(fd, static_cast<uint8_t*>(data) + readBytes, size - readBytes, 0);
        if (len < 0) {
            return -errno;
        }

        if (len == 0) {
            LOG_DBG() << "Connection closed by peer";

            return -ECONNRESET;
        }

        readBytes += len;
    }

    LOG_DBG() << "Read from socket: readBytes=" << readBytes;

    return readBytes;
}

int Socket::WriteToSocket(int fd, const void* data, size_t size)
{
    ssize_t writtenBytes = 0;
    while (writtenBytes < static_cast<ssize_t>(size)) {
        ssize_t len = send(fd, static_cast<const uint8_t*>(data) + writtenBytes, size - writtenBytes, 0);
        if (len < 0) {
            return -errno;
        }

        if (len == 0) {
            LOG_DBG() << "Connection closed by peer";

            return -ECONNRESET;
        }

        writtenBytes += len;
    }

    LOG_DBG() << "Written to socket: writtenBytes=" << writtenBytes;

    return writtenBytes;
}

} // namespace aos::zephyr::communication
