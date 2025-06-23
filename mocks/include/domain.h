/* SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2025 EPAM Systems
 */

#ifndef XENLIB_XEN_DOMAIN_H
#define XENLIB_XEN_DOMAIN_H

#include <stdint.h>

extern "C" {

#define CONTAINER_NAME_SIZE 64

struct xen_domain {
    uint32_t domid;
    char     name[CONTAINER_NAME_SIZE];
};

} // extern "C"

#endif /* XRUN_H_ */
