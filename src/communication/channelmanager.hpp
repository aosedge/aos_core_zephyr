/*
 * Copyright (C) 2024 Renesas Electronics Corporation.
 * Copyright (C) 2024 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef COMMUNICATION_HPP_
#define COMMUNICATION_HPP_

#include <aos/common/tools/allocator.hpp>
#include <aos/common/tools/map.hpp>
#include <aos/common/tools/memory.hpp>

#include "channel.hpp"
#include "transport.hpp"

namespace aos::zephyr::communication {

/**
 * Channel manager interface.
 */
class ChannelManagerItf {
public:
    /**
     * Create channel with dedicated port.
     *
     * @param port port to bind channel.
     * @return aos::RetWithError<ChannelItf*>
     */
    virtual RetWithError<ChannelItf*> CreateChannel(uint32_t port) = 0;

    /**
     * Deletes channel.
     *
     * @param port port channel is bound to.
     * @return Error.
     */
    virtual Error DeleteChannel(uint32_t port) = 0;

    /**
     * Closes communication channel.
     *
     * @return aos::Error.
     */
    virtual Error Close() = 0;

    /**
     * Destructor.
     */
    virtual ~ChannelManagerItf() = default;
};

/**
 * Channel manager instance.
 */
class ChannelManager : public ChannelManagerItf, public CommunicationItf {
public:
    /**
     * Constructor.
     */
    ChannelManager() = default;

    /**
     * Initializes channel manager.
     *
     * @param transport communication transport.
     * @return Error.
     */
    Error Init(TransportItf& transport);

    /**
     * Create channel with dedicated port.
     *
     * @param port port to bind channel.
     * @return aos::RetWithError<ChannelItf*>
     */
    RetWithError<ChannelItf*> CreateChannel(uint32_t port) override;

    /**
     * Deletes channel.
     *
     * @param port port channel is bound to.
     * @return aos::Error
     */
    Error DeleteChannel(uint32_t port) override;

    /**
     * Closes communication channel.
     *
     * @return aos::Error.
     */
    Error Close() override;

    /**
     * Connects to communication channel.
     *
     * @return aos::Error.
     */
    Error Connect() override;

    /**
     * Write data to channel.
     *
     * @param port port to write data to.
     * @param data data to write.
     * @param size size of data.
     * @return uint32_t number of bytes written.
     */
    int Write(uint32_t port, const void* data, size_t size) override;

    /**
     * Checks if channel manager is connected.
     *
     * @return bool true if connected, false otherwise.
     */
    bool IsConnected() const override;

private:
    static constexpr int    cMaxChannels       = 4;
    static constexpr auto   cChanAllocatorSize = cMaxChannels * sizeof(Channel);
    static constexpr auto   cReconnectPeriod   = 2 * Time::cSeconds;
    static constexpr size_t cReadBufferSize    = 65 * 1024; // 65KB

    Error Run();
    Error HandleRead();
    void  CloseChannels();
    Error TryConnect();
    Error WaitTimeout();
    Error ReadFromTransport(AosProtocolHeader& header);
    Error ProcessData(const AosProtocolHeader& header);

    aos::RetWithError<AosProtocolHeader> PrepareHeader(uint32_t port, const aos::Array<uint8_t>& data);

    StaticAllocator<cChanAllocatorSize>                   mChanAllocator;
    TransportItf*                                         mTransport {};
    StaticMap<uint32_t, SharedPtr<Channel>, cMaxChannels> mChannels;

    aos::Thread<>       mThread;
    aos::Mutex          mMutex;
    bool                mClose {false};
    ConditionalVariable mCondVar;
    static Mutex        sWriteMutex;
    uint8_t             mTmpReadBuffer[cReadBufferSize] {};
};

} // namespace aos::zephyr::communication

#endif
