/*
 * Copyright (c) 2023 EPAM Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>

#ifdef CONFIG_DOMD_ENABLE
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
#endif

#ifdef CONFIG_DOMU_ENABLE
.section .data.img_domu,"a"
GDATA(__img_domu_start)
GDATA(__img_domu_end)
__img_domu_start:
.incbin CONFIG_DOMU_BIN_PATH
__img_domu_end:

.section .data.dtb_domu,"a"
GDATA(__dtb_domu_start)
GDATA(__dtb_domu_end)
__dtb_domu_start:
.align 8
.incbin CONFIG_DOMU_DTB_PATH
__dtb_domu_end:
#endif
