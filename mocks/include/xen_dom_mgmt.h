/* SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2025 EPAM Systems
 */

#ifndef XENLIB_XEN_DOM_MGMT_H
#define XENLIB_XEN_DOM_MGMT_H

#include <domain.h>
#include <stdint.h>

extern "C" {

#define CONTAINER_NAME_SIZE 64

struct xen_domain* get_domain(uint32_t domid);
uint32_t           find_domain_by_name(char* arg);

} // extern "C"

#endif /* XRUN_H_ */
