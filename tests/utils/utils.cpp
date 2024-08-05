/*
 * Copyright (C) 2024 Renesas Electronics Corporation.
 * Copyright (C) 2024 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <aos/common/tools/string.hpp>

#include "utils.hpp"

/***********************************************************************************************************************
 * Static
 **********************************************************************************************************************/

aos::StaticString<256> sErrorStr;

/***********************************************************************************************************************
 * Public
 **********************************************************************************************************************/

const char* AosErrorToStr(const aos::Error& err)
{
    sErrorStr.Convert(err);

    return sErrorStr.CStr();
}
