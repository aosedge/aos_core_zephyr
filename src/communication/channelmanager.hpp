/*
 * Copyright (C) 2024 Renesas Electronics Corporation.
 * Copyright (C) 2024 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef COMMUNICATION_HPP_
#define COMMUNICATION_HPP_

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
     * @return aos::RetWithError<ChannelItf&>
     */
    virtual aos::RetWithError<ChannelItf&> CreateChannel(uint32_t port) = 0;

    /**
     * Deletes channel.
     *
     * @param port port channel is bound to.
     * @return aos::Error
     */
    virtual aos::Error DeleteChannel(uint32_t port) = 0;

    /**
     * Destructor.
     */
    virtual ~ChannelManagerItf() { }
};

/**
 * Channel manager instance.
 */
class ChannelManager : public ChannelManagerItf {
public:
    /**
     * Initializes channel manager.
     *
     * @param transport communication transport.
     * @return * aos::Error
     */
    aos::Error Init(TransportItf& transport);

    /**
     * Create channel with dedicated port.
     *
     * @param port port to bind channel.
     * @return aos::RetWithError<ChannelItf&>
     */
    aos::RetWithError<ChannelItf&> CreateChannel(uint32_t port) override;

    /**
     * Deletes channel.
     *
     * @param port port channel is bound to.
     * @return aos::Error
     */
    aos::Error DeleteChannel(uint32_t port) override;
};

} // namespace aos::zephyr::communication

#endif
