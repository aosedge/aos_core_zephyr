#include <ff.h>
#include <storage.h>

#include <zephyr/fs/fs.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/storage/disk_access.h>

#include "mount.h"

#define DISK_DRIVE_NAME   CONFIG_SDMMC_VOLUME_NAME
#define DISK_TMP_MOUNT_PT "/2:"

static FATFS fat_fs_aos;
static FATFS fat_fs_tmp;

/* mounting info */
static struct fs_mount_t mp_aos = {
    .type      = FS_FATFS,
    .fs_data   = &fat_fs_aos,
    .mnt_point = CONFIG_AOS_DISK_MOUNT_POINT,
};

static struct fs_mount_t mp_tmp = {
    .type      = FS_FATFS,
    .fs_data   = &fat_fs_tmp,
    .mnt_point = DISK_TMP_MOUNT_PT,
};

#if defined(CONFIG_FS_MULTI_PARTITION)
PARTITION VolToPart[FF_VOLUMES] = {
    [0] = {3, 1}, /* "0:" ==> 1st partition on the pd#3 (SD) */
    [1] = {3, 2}, /* "1:" ==> 2nd partition on the pd#3 (SD) */
    [2] = {0, 0}, /* "2:" ==> 1rd partition on the pd#0 (RAM) */
    [3] = {0, -1},
    [4] = {0, -1},
    [5] = {0, -1},
    [6] = {0, -1},
    [7] = {0, -1},
};
#endif /* CONFIG_FS_MULTI_PARTITION */

int fatfs_mount()
{
    printk("Mounting fatfs...");

    int ret = disk_access_init(DISK_DRIVE_NAME);
    if (ret) {
        printk("[fatfs] Access init failed (%d)\n", ret);

        return ret;
    }

    ret = fs_mount(&mp_aos);
    if (ret) {
        printk("[fatfs] Aos disk mount failed (%d)\n", ret);

        return ret;
    }

    printk("[fatfs] Aos disk mounted at %s\n", CONFIG_AOS_DISK_MOUNT_POINT);

    ret = fs_mount(&mp_tmp);
    if (ret) {
        printk("[fatfs] TMP disk mount failed (%d)\n", ret);

        return ret;
    }

    printk("[fatfs] TMP disk mounted at %s\n", DISK_TMP_MOUNT_PT);

    ret = fs_mkdir(DISK_TMP_MOUNT_PT "/tmp");
    if (ret) {
        printk("[fatfs] TMP folder create failed (%d)\n", ret);

        return ret;
    }

    return 0;
}
