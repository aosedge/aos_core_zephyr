/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef NEW_HPP_
#define NEW_HPP_

#include <stddef.h>

inline void* operator new(size_t, void* p)
{
    return p;
}

#endif
