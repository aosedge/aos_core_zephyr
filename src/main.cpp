/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/__assert.h>
#include <zephyr/sys/printk.h>

#include <aos/common/version.hpp>

#include "app.hpp"
#include "logger.hpp"
#include "version.hpp"

int main(void)
{
    printk("*** Aos zephyr application: %s ***\n", AOS_ZEPHYR_APP_VERSION);
    printk("*** Aos core library: %s ***\n", AOS_CORE_VERSION);

    Logger::Init();

    auto& app = App::Get();

    auto err = app.Init();
    __ASSERT(!err.IsNone(), "Error initializing application: %s", err.ToString());

    return 0;
}
