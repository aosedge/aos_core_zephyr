/*
 * Copyright (c) 2024 EPAM Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <domain.h>
#include <domains/dom_runner.h>
#include <string.h>
#include <xen_dom_mgmt.h>
#include <xl_parser.h>

#include <zephyr/logging/log.h>
#include <zephyr/xen/public/domctl.h>

LOG_MODULE_DECLARE(domains);

extern char __img_domu_start[];
extern char __img_domu_end[];
extern char __dtb_domu_start[];
extern char __dtb_domu_end[];

static int load_domu_image(uint8_t* buf, size_t bufsize, uint64_t image_load_offset, void* image_info)
{
    ARG_UNUSED(image_info);
    memcpy(buf, __img_domu_start + image_load_offset, bufsize);
    return 0;
}

static ssize_t get_domu_image_size(void* image_info, uint64_t* size)
{
    ARG_UNUSED(image_info);
    // cppcheck-suppress comparePointers
    *size = __img_domu_end - __img_domu_start;
    return 0;
}

static const char* domu_backend_configs[] = {
    "disk=['backend=1, vdev=xvda, access=rw, target=/dev/sda7']",
    "vif=['backend=1,bridge=xenbr0,mac=08:00:27:ff:cb:ce,ip=172.44.0.2 255.255.255.0 172.44.0.1']",
};

struct xen_domain_cfg domu_cfg = {
    .mem_kb = 0x40000, /* 256Mb */

    .flags               = (XEN_DOMCTL_CDF_hvm | XEN_DOMCTL_CDF_hap | XEN_DOMCTL_CDF_iommu),
    .max_evtchns         = 10,
    .max_vcpus           = 1,
    .gnt_frames          = 32,
    .max_maptrack_frames = 1,

#ifdef CONFIG_GIC_V3
    .gic_version = XEN_DOMCTL_CONFIG_GIC_V3,
#else
    .gic_version = XEN_DOMCTL_CONFIG_GIC_V2,
#endif

    .tee_type = XEN_DOMCTL_CONFIG_TEE_NONE,

    .load_image_bytes = load_domu_image,
    .get_image_size   = get_domu_image_size,
    .image_info       = NULL,

    .dtb_start = __dtb_domu_start,
    .dtb_end   = __dtb_domu_end,

    .cmdline = "console=hvc0 clk_ignore_unused root=/dev/xvda rw rootwait ignore_loglevel",
};

int domu_start(void)
{
    int i, rc, domid = 0;

    for (i = 0; i < ARRAY_SIZE(domu_backend_configs); i++) {
        rc = parse_one_record_and_fill_cfg(domu_backend_configs[i], &domu_cfg.back_cfg);

        if (rc) {
            LOG_ERR("Failed to parse backend configuration #%d, rc = %d!", i, rc);
            return rc;
        }
    }

    domid = domain_create(&domu_cfg, domid);
    if (domid < 0) {
        LOG_ERR("Failed to create DomU, rc = %d", rc);
        return rc;
    }

    rc = domain_post_create(&domu_cfg, domid);
    if (rc) {
        LOG_ERR("Failed to setup backends for DomU, rc = %d", rc);
        return rc;
    }

    return 0;
}
