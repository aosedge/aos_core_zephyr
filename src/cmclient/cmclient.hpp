/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CMCLIENT_HPP_
#define CMCLIENT_HPP_

#include <aos/common/error.hpp>
#include <aos/common/noncopyable.hpp>

class CMClient : private aos::NonCopyable {
public:
    aos::Error Init();
};

#endif
