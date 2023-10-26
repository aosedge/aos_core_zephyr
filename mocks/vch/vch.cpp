/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vch.h"

int vch_open(domid_t domid, const char* path, size_t min_rs, size_t min_ws, struct vch_handle* handle)
{
    return 0;
}

int vch_connect(domid_t domid, const char* path, struct vch_handle* handle)
{
    return 0;
}

void vch_close(struct vch_handle* handle)
{
}

int vch_read(struct vch_handle* handle, void* buf, size_t size)
{
    while (true) { }

    return 0;
}

int vch_write(struct vch_handle* handle, const void* buf, size_t size)
{
    return size;
}
