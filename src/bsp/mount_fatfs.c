#include <ff.h>
#include <storage.h>

#include <zephyr/fs/fs.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/storage/disk_access.h>

#include "mount.h"

#define DISK_DRIVE_NAME CONFIG_SDMMC_VOLUME_NAME

static FATFS fat_fs;

/* mounting info */
static struct fs_mount_t mp = {
    .type    = FS_FATFS,
    .fs_data = &fat_fs,
};

#if defined(CONFIG_FS_MULTI_PARTITION)
PARTITION VolToPart[FF_VOLUMES] = {
    [0] = {3, 1}, /* "0:" ==> 1st partition on the pd#0 */
    [1] = {3, 2}, /* "1:" ==> 2nd partition on the pd#0 */
    [2] = {0, -1}, /* "2:" ==> 3rd partition on the pd#0 */
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

    uint32_t block_count;
    uint32_t block_size;

    int ret = disk_access_init(DISK_DRIVE_NAME);
    if (ret) {
        printk("[fatfs] access init failed (%d)\n", ret);

        return ret;
    }

    ret = disk_access_ioctl(DISK_DRIVE_NAME, DISK_IOCTL_GET_SECTOR_COUNT, &block_count);
    if (ret) {
        printk("[fatfs] get sector count failed (%d)\n", ret);

        return ret;
    }

    ret = disk_access_ioctl(DISK_DRIVE_NAME, DISK_IOCTL_GET_SECTOR_SIZE, &block_size);
    if (ret) {
        printk("[fatfs] get sector size failed (%d)\n", ret);

        return ret;
    }

    mp.mnt_point = CONFIG_AOS_DISK_MOUNT_POINT;

    ret = fs_mount(&mp);
    if (ret) {
        printk("[fatfs] mount failed (%d)\n", ret);

        return ret;
    }

    uint64_t memory_size_mb = (uint64_t)block_count * block_size;

    printk("[fatfs] Mounted %s, block count=%u, Sector size=%u, Memory Size(MB)=%u\n", CONFIG_AOS_DISK_MOUNT_POINT,
        block_count, block_size, (uint32_t)(memory_size_mb >> 20));

    return 0;
}
