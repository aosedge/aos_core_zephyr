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

#if defined(CONFIG_FS_LITTLEFS_BLK_DEV) && defined(CONFIG_FILE_SYSTEM_LITTLEFS)
#include <zephyr/fs/littlefs.h>
FS_LITTLEFS_DECLARE_DEFAULT_CONFIG(lfs_data);
static struct fs_mount_t littlefs_mnt = {
    .type = FS_LITTLEFS,
    .mnt_point = "/aos:",
    .fs_data = &lfs_data,
    .storage_dev = (void *)CONFIG_SDMMC_VOLUME_NAME,
    .flags = FS_MOUNT_FLAG_USE_DISK_ACCESS
};

/*
 * NOTE: Unmount function should not be implemented.
 * This because we have one-shot application and we do not expect
 * to have some reset of shutdown procedure.
 */
static int mount_mmc()
{
    int rc = fs_mount(&littlefs_mnt);

    if (rc != 0) {
        printk("Error mounting %s:%d\n", littlefs_mnt.mnt_point, rc);
        return -ENOEXEC;
    }

    return 0;
}
#else
#define mount_mmc()  0
#endif /* CONFIG_FS_LITTLEFS_BLK_DEV && CONFIG_FILE_SYSTEM_LITTLEFS */

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

    int rc = mount_mmc();
    if (rc != 0) {
        return rc;
    }

#if defined(CONFIG_SOC_SERIES_RCAR_GEN4) || defined(CONFIG_SOC_SERIES_RCAR_GEN3)
    rc = domain_create(&domd_cfg, 1);
    if (rc) {
        printk("failed to create domain (%d)\n", rc);
        return rc;
    }
#endif /* CONFIG_SOC_SERIES_RCAR_GEN4 || CONFIG_SOC_SERIES_RCAR_GEN3 */

    Logger::Init();

    auto& app = App::Get();

    auto err = app.Init();
    __ASSERT(err.IsNone(), "Error initializing application: %s", aos::StaticString<128>().Convert(err).CStr());

    return 0;
}
