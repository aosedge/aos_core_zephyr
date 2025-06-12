/*
 * Copyright (c) 2023 EPAM Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "dom_runner.h"
#include <domain.h>
#include <xen_dom_mgmt.h>
#include <zephyr/logging/log.h>

#ifdef CONFIG_DOMU_ENABLE
#include "domu/domu_runner.h"
#endif

LOG_MODULE_REGISTER(domains);

/* This is needed until we will not use domain names for backend confs */
#define DOMID_DOMD 1

#ifdef CONFIG_SOC_SERIES_RCAR_GEN4
char* domd_cfgname = SPIDER_DOMD_NAME;
#elif CONFIG_BOARD_RCAR_SALVATOR_XS_M3
char* domd_cfgname = SALVATOR_XS_DOMD_NAME;
#elif CONFIG_BOARD_RCAR_H3ULCB_CA57
char* domd_cfgname = H3ULCB_DOMD_NAME;
#else
char* domd_cfgname = "not_set";
#endif

int create_domains(void)
{
    int rc = 0; // cppcheck-suppress unreadVariable

#ifdef CONFIG_DOMD_ENABLE
    struct xen_domain_cfg* dom_cfg = domain_find_config(domd_cfgname);
    if (!dom_cfg) {
        LOG_ERR("Failed to find Domain-D configuration");
        return -ENOENT;
    }
    rc = domain_create(dom_cfg, DOMID_DOMD);
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
