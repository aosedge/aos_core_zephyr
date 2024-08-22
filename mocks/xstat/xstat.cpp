/*
 * Copyright (c) 2023 EPAM Systems
 *
 * work based on the libxenstat library from xen-tools
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "xstat.h"
#include <string.h>

static constexpr auto MB = 1024 * 1024;

int xstat_getstat(struct xenstat* stat)
{
    stat->num_cpus = 1;
    stat->cpu_hz   = 1000;
    stat->tot_mem  = 1024 * MB;
    stat->free_mem = stat->tot_mem - 100 * MB;
    strcpy(stat->xen_version, "1.0");

    return 0;
}

int xstat_getdominfo(struct xenstat_domain* info, uint16_t first, uint16_t num)
{
    info->id = first;
    strcpy(info->name, "DOM0");

    info->state     = 0;
    info->cpu_ns    = 1;
    info->num_vcpus = 0;
    info->cur_mem   = 1000;
    info->max_mem   = 1000 * MB;
    info->ssid      = 0;

    return 0;
}

int xstat_getvcpu(struct xenstat_vcpu* info, uint16_t dom, uint16_t vcpu)
{
    return 0;
}
