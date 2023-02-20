/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "cmclient.hpp"
#include "log.hpp"

aos::Error CMClient::Init()
{
    LOG_DBG() << "Hello form CM client";

    return aos::ErrorEnum::eNone;
}
