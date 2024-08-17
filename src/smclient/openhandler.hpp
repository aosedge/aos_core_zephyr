/*
 * Copyright (C) 2024 Renesas Electronics Corporation.
 * Copyright (C) 2024 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef OPENHANDLER_HPP_
#define OPENHANDLER_HPP_

#include <proto/servicemanager/v4/servicemanager.pb.h>

#include "clocksync/clocksync.hpp"
#include "communication/channel.hpp"
#include "communication/pbhandler.hpp"

namespace aos::zephyr::smclient {

/**
 * SM open handler.
 */
class OpenHandler : public communication::PBHandler<servicemanager_v4_SMIncomingMessages_size,
                        servicemanager_v4_SMOutgoingMessages_size> {
public:
    /**
     * Initializes open handler.
     *
     * @param channel communication channel.
     * @param clockSync clock sync instance.
     * @return Error.
     */
    Error Init(communication::ChannelItf& channel, clocksync::ClockSyncItf& clockSync);

    /**
     * Sends clock sync request.
     *
     * @return Error.
     */
    Error SendClockSyncRequest();

    /**
     * Destructor.
     */
    ~OpenHandler();

private:
    void  OnConnect() override;
    void  OnDisconnect() override;
    Error ReceiveMessage(const Array<uint8_t>& data) override;
    Error ProcessClockSync(const servicemanager_v4_ClockSync& pbClockSync);

    clocksync::ClockSyncItf*             mClockSync {};
    servicemanager_v4_SMOutgoingMessages mOutgoingMessages;
    servicemanager_v4_SMIncomingMessages mIncomingMessages;
};

} // namespace aos::zephyr::smclient

#endif
