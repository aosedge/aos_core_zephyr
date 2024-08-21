#include <zephyr/sys/printk.h>

#include "mount.h"

int mount_fs()
{
    int ret = 0;

#if defined(CONFIG_FILE_SYSTEM_LITTLEFS)
    ret = littlefs_mount();
    if (ret != 0) {
        printk("littlefs mount failed (%d)\n", ret);

        return ret;
    }

#elif defined(CONFIG_FAT_FILESYSTEM_ELM)
    ret = fatfs_mount();
    if (ret != 0) {
        printk("fatfs mount failed (%d)\n", ret);

        return ret;
    }
#endif

    return ret;
}
