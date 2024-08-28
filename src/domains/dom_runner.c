/*
 * Copyright (c) 2023 EPAM Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <domain.h>
#include <xen_dom_mgmt.h>
#include <zephyr/logging/log.h>

#ifdef CONFIG_DOMU_ENABLE
#include "domu/domu_runner.h"
#endif

LOG_MODULE_REGISTER(domains);

/* This is needed until we will not use domain names for backend confs */
#define DOMID_DOMD 1

extern struct xen_domain_cfg domd_cfg;

int create_domains(void)
{
    int rc = 0; // cppcheck-suppress unreadVariable

#ifdef CONFIG_DOMD_ENABLE
    rc = domain_create(&domd_cfg, DOMID_DOMD);
    if (rc < 0) {
        LOG_ERR("Failed to start Domain-D, rc = %d", rc);
        return rc;
    } else if (rc != DOMID_DOMD) {
        /* Since this value is used for backend configs, it is critical error for us */
        LOG_ERR("Failed to start Domain-D with specified domid");
        return rc;
    }
#endif

#ifdef CONFIG_DOMU_ENABLE
    rc = domu_start();
    if (rc < 0) {
        LOG_ERR("Failed to start Domain-U, rc = %d", rc);
        return rc;
    }
#endif

    return 0;
}
