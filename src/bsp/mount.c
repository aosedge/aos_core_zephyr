/*#include <zephyr/fs/fs.h>
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
}*/

#include <zephyr/kernel.h>
#include <zephyr/fs/fs.h>
#include <zephyr/logging/log.h>
#include <zephyr/storage/disk_access.h>
#include <ff.h>
#include <storage.h>

#define DISK_DRIVE_NAME CONFIG_SDMMC_VOLUME_NAME
#define DISK_MOUNT_PT "/1:"
#define DISK_BIN_PATH DISK_MOUNT_PT"/"

static FATFS fat_fs;
/* mounting info */
static struct fs_mount_t mp = {
	.type = FS_FATFS,
	.fs_data = &fat_fs,
};

#if defined(CONFIG_FS_MULTI_PARTITION)
PARTITION VolToPart[FF_VOLUMES] = {
    [0] = {3, 1},     /* "0:" ==> 1st partition on the pd#0 */
    [1] = {3, 2},     /* "1:" ==> 2nd partition on the pd#0 */
    [2] = {0, -1},     /* "2:" ==> 3rd partition on the pd#0 */
    [3] = {0, -1},
    [4] = {0, -1},
    [5] = {0, -1},
    [6] = {0, -1},
    [7] = {0, -1},
};
#endif /* CONFIG_FS_MULTI_PARTITION */

/*int storage_image_kernel_read(uint8_t *buf, size_t bufsize, uint64_t offset, void *image_info)
{
	struct dom0_domain_cfg *dom_cfg = image_info;

	LOG_INF("storage: file read %s size: %zd", dom_cfg->image_kernel_path, bufsize);
	return xrun_read_file(dom_cfg->image_kernel_path, buf, bufsize, offset);
}

ssize_t storage_image_kernel_get_size(void *image_info, uint64_t *size)
{
	struct dom0_domain_cfg *dom_cfg = image_info;
	ssize_t r_size;

	r_size = xrun_get_file_size(dom_cfg->image_kernel_path);
	if (r_size >= 0) {
		*size = r_size;
		return 0;
	}

	return r_size;
}

int storage_image_dt_read(uint8_t *buf, size_t bufsize, uint64_t offset, void *image_info)
{
	struct dom0_domain_cfg *dom_cfg = image_info;

	LOG_INF("storage: file read %s size: %zd", dom_cfg->image_dt_path, bufsize);
	return xrun_read_file(dom_cfg->image_dt_path, buf, bufsize, offset);
}

int storage_image_dt_get_size(void *image_info, size_t *size)
{
	struct dom0_domain_cfg *dom_cfg = image_info;
	ssize_t r_size;

	r_size = xrun_get_file_size(dom_cfg->image_dt_path);
	if (r_size >= 0) {
		*size = r_size;
		return 0;
	}

	return r_size;
}*/

int storage_init(void)
{
	uint64_t memory_size_mb;
	struct fs_dir_t dirp;
	uint32_t block_count;
	uint32_t block_size;

	int ret;

	ret = disk_access_init(DISK_DRIVE_NAME);
	if (ret) {
//		LOG_ERR("storage: init failed (%d)", ret);
		goto exit_err;
	}

	ret = disk_access_ioctl(DISK_DRIVE_NAME, DISK_IOCTL_GET_SECTOR_COUNT, &block_count);
	if (ret) {
//		LOG_ERR("storage: get sector count failed (%d)", ret);
		goto exit_err;
	}

	ret = disk_access_ioctl(DISK_DRIVE_NAME, DISK_IOCTL_GET_SECTOR_SIZE, &block_size);
	if (ret) {
//		LOG_ERR("storage: get sector size failed (%d)", ret);
		goto exit_err;
	}

	memory_size_mb = (uint64_t)block_count * block_size;
//	LOG_INF("storage: block count %u, Sector size %u, Memory Size(MB) %u", block_count,
//		block_size, (uint32_t)(memory_size_mb >> 20));

	mp.mnt_point = DISK_MOUNT_PT;
	printf("Mounted =================== %s\n", DISK_MOUNT_PT);
	ret = fs_mount(&mp);
	if (ret) {
//		LOG_ERR("storage: mount failed (%d)", ret);
		goto exit_err;
	}
//	LOG_INF("storage: mounted, binaries folder %s", DISK_BIN_PATH);
	printf("Mounted ===================\n");
	/* Verify binaries folder */
	fs_dir_t_init(&dirp);
	ret = fs_opendir(&dirp, DISK_BIN_PATH);
	if (ret) {
//		LOG_ERR("storage: error opening dir %s [%d]\n", DISK_BIN_PATH, ret);
		return ret;
	}

	fs_closedir(&dirp);

exit_err:
	return ret;
}
