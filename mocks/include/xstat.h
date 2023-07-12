/*
 * Copyright (c) 2023 EPAM Systems
 *
 * work based on the libxenstat library from xen-tools
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _XSTAT_H
#define _XSTAT_H

#include <stdint.h>

extern "C" {

#define CONTAINER_NAME_SIZE 64
#define VERSION_SIZE        32

struct xenstat_vcpu {
    bool               online;
    unsigned long long cpu_time;
};

struct xenstat_domain {
    unsigned short id;
    char           name[CONTAINER_NAME_SIZE];
    /* XEN_DOMCTL_* flags are defined in include/zephyr/xen/public/domctl.h */
    unsigned int       state;
    unsigned long long cpu_ns;
    unsigned int       num_vcpus; /* No. vcpus configured for domain */
    unsigned long long cur_mem; /* Current memory reservation */
    unsigned long long max_mem; /* Total memory allowed */
    unsigned int       ssid;
};

struct xenstat {
    unsigned int       num_cpus;
    unsigned long long cpu_hz;
    unsigned long long tot_mem;
    unsigned long long free_mem;
    char               xen_version[VERSION_SIZE]; /* xen version running on this node */
};

int xstat_getstat(struct xenstat* stat);
int xstat_getdominfo(struct xenstat_domain* info, uint16_t first, uint16_t num);
int xstat_getvcpu(struct xenstat_vcpu* info, uint16_t dom, uint16_t vcpu);

} // extern "C"

#endif
