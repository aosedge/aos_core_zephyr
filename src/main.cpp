/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/__assert.h>
#include <zephyr/sys/printk.h>

#include <aos/common/tools/string.hpp>
#include <aos/common/version.hpp>

#include "app/app.hpp"
#include "logger/logger.hpp"
#include "version.hpp"

#if defined(CONFIG_SOC_SERIES_RCAR_GEN4) || defined(CONFIG_SOC_SERIES_RCAR_GEN3)
extern "C" {
#include <xen_dom_mgmt.h>

extern struct xen_domain_cfg domd_cfg;
}
#endif /* CONFIG_SOC_SERIES_RCAR_GEN4 || CONFIG_SOC_SERIES_RCAR_GEN3 */

int main(void)
{
    printk("*** Aos zephyr application: %s ***\n", AOS_ZEPHYR_APP_VERSION);
    printk("*** Aos core library: %s ***\n", AOS_CORE_VERSION);

#if defined(CONFIG_SOC_SERIES_RCAR_GEN4) || defined(CONFIG_SOC_SERIES_RCAR_GEN3)
    int rc = domain_create(&domd_cfg, 1);
    if (rc) {
        printk("failed to create domain (%d)\n", rc);
    }
#endif /* CONFIG_SOC_SERIES_RCAR_GEN4 || CONFIG_SOC_SERIES_RCAR_GEN3 */

    Logger::Init();

    auto& app = App::Get();

    auto err = app.Init();
    __ASSERT(err.IsNone(), "Error initializing application: %s", aos::StaticString<128>().Convert(err).CStr());

    return 0;
}
