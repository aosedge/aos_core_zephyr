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

#if !defined(CONFIG_NATIVE_APPLICATION)
#include "bsp/mount.h"
#include "bsp/reboot.h"
#include "domains/dom_runner.h"
#endif

int main(void)
{
    printk("*** Aos zephyr application: %s ***\n", AOS_ZEPHYR_APP_VERSION);
    printk("*** Aos core library: %s ***\n", AOS_CORE_VERSION);
    printk("*** Aos core size: %lu ***\n", sizeof(App));

#if !defined(CONFIG_NATIVE_APPLICATION)
    if (auto ret = littlefs_mount()) {
        printk("Failed to mount littlefs (%d)\n", ret);
    }

    reboot_watcher_init();

    if (auto ret = create_domains()) {
        printk("Failed to create domain (%d)\n", ret);
    }
#endif

    Logger::Init();

    auto& app = App::Get();

    auto err = app.Init();
    __ASSERT(
        err.IsNone(), "Error initializing application: %s (%s:%d)", err.Message(), err.FileName(), err.LineNumber());

    return 0;
}
