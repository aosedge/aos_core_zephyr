/*
 * Copyright (C) 2024 Renesas Electronics Corporation.
 * Copyright (C) 2024 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <aos/common/tools/string.hpp>
#include <aos/common/tools/thread.hpp>

#include "utils.hpp"

namespace aos::zephyr::utils {

/***********************************************************************************************************************
 * Static
 **********************************************************************************************************************/

static StaticString<256> sErrorStr;
static Mutex             sMutex;

/***********************************************************************************************************************
 * Public
 **********************************************************************************************************************/

const char* ErrorToCStr(const Error& err)
{
    LockGuard lock {sMutex};

    sErrorStr.Convert(err);

    return sErrorStr.CStr();
}

} // namespace aos::zephyr::utils
