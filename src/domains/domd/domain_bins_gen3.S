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
.incbin	 "ipl_gen3.bin"
__img_ipl_end:

.section .data.dtb_ipl,"a"
GDATA(__dtb_ipl_start)
GDATA(__dtb_ipl_end)
__dtb_ipl_start:
.align 8
#if defined(CONFIG_BOARD_RCAR_SALVATOR_XS_M3)
.incbin	 "ipl_salvator_m3.dtb"
#elif defined(CONFIG_BOARD_RCAR_H3ULCB_CA57)
.incbin	 "ipl_h3ulcb.dtb"
#else
#error "No Domain-D dtb for target board!"
#endif
__dtb_ipl_end:
