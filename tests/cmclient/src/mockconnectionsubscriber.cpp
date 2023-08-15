
/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mockconnectionsubscriber.hpp"

void MockConnectionSubscriber::OnConnect()
{
    connected = true;
}

void MockConnectionSubscriber::OnDisconnect()
{
    connected = false;
}

bool MockConnectionSubscriber::IsConnected() const
{
    return connected;
}
