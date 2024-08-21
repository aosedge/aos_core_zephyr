#ifndef MOUNT_H_
#define MOUNT_H_

#ifdef __cplusplus
extern "C" {
#endif

int littlefs_mount();
int fatfs_mount();
int mount_fs();

#ifdef __cplusplus
}
#endif

#endif
