/* SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2025 EPAM Systems
 */

#include "xen_console.h"

int set_console_feed_cb(struct xen_domain* domain, on_console_feed_cb_t cb, void* cb_data)
{
    (void)domain;
    (void)cb;
    (void)cb_data;

    return 0;
}
