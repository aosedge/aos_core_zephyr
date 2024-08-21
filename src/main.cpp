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
#include "utils/utils.hpp"

// cppcheck-suppress missingInclude
#include "version.hpp"

#if !defined(CONFIG_NATIVE_APPLICATION)
#include <tee_supplicant.h>

#include "bsp/mount.h"
#include "bsp/reboot.h"
#include "domains/dom_runner.h"
#endif

using namespace aos::zephyr;

int main(void)
{
    printk("*** Aos zephyr application: %s ***\n", AOS_ZEPHYR_APP_VERSION);
    printk("*** Aos core library: %s ***\n", AOS_CORE_VERSION);
    printk("*** Aos core size: %lu ***\n", sizeof(app::App));

#if !defined(CONFIG_NATIVE_APPLICATION)
    auto ret = mount_fs();
    __ASSERT(ret == 0, "Error mounting FS: %s [%d]", strerror(ret), ret);

    ret = TEE_SupplicantInit();
    __ASSERT(ret == 0, "Error initializing TEE supplicant: %s [%d]", strerror(ret), ret);

    reboot_watcher_init();

    ret = create_domains();
    __ASSERT(ret == 0, "Error creating domains: %s [%d]", strerror(ret), ret);
#endif

    logger::Logger::Init();

    auto& app = aos::zephyr::app::App::Get();

    auto err = app.Init();
    __ASSERT(err.IsNone(), "Error initializing application: %s", utils::ErrorToCStr(err));

    return 0;
}
