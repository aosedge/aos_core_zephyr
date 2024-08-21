#include <zephyr/fs/fs.h>
#include <zephyr/fs/littlefs.h>

#include "mount.h"

FS_LITTLEFS_DECLARE_DEFAULT_CONFIG(lfsfs);
static struct fs_mount_t mp = {
    .type    = FS_LITTLEFS,
    .fs_data = &lfsfs,
    .flags   = FS_MOUNT_FLAG_USE_DISK_ACCESS,
};

int littlefs_mount()
{
    static const char* disk_mount_pt = CONFIG_AOS_DISK_MOUNT_POINT;
    static const char* disk_pdrv     = CONFIG_MMC_VOLUME_NAME;

    printk("Mounting littlefs...");

    mp.storage_dev = (void*)disk_pdrv;
    mp.mnt_point   = disk_mount_pt;

    return fs_mount(&mp);
}
