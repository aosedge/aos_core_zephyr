/*
 * Copyright (c) 2023 EPAM Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>

.section .data.img_ipl,"a"
GDATA(__img_ipl_start)
GDATA(__img_ipl_end)
__img_ipl_start:
.incbin CONFIG_DOMD_UBOOT_PATH
__img_ipl_end:

.section .data.dtb_ipl,"a"
GDATA(__dtb_ipl_start)
GDATA(__dtb_ipl_end)
__dtb_ipl_start:
.align 8
.incbin CONFIG_DOMD_DTB_PATH
__dtb_ipl_end:
