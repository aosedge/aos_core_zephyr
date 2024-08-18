/*
 * Copyright (C) 2024 Renesas Electronics Corporation.
 * Copyright (C) 2024 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "channelmanager.hpp"

namespace aos::zephyr::communication {

/***********************************************************************************************************************
 * Public
 **********************************************************************************************************************/

Error ChannelManager::Init(TransportItf& transport)
{
    return ErrorEnum::eNone;
}

RetWithError<ChannelItf*> ChannelManager::CreateChannel(uint32_t port)
{
    return {nullptr, ErrorEnum::eNotSupported};
}

Error ChannelManager::DeleteChannel(uint32_t port)
{
    return ErrorEnum::eNone;
}

} // namespace aos::zephyr::communication
