/*
 * Copyright (C) 2024 Renesas Electronics Corporation.
 * Copyright (C) 2024 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>

.section .data.aos_root_ca,"a"
GDATA(__aos_root_ca_start)
GDATA(__aos_root_ca_end)
.align 8
__aos_root_ca_start:
.incbin  CONFIG_AOS_ROOT_CA_PATH
.byte 0x00
__aos_root_ca_end:
