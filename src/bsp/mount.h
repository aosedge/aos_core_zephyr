#ifndef MOUNT_H_
#define MOUNT_H_

#ifdef __cplusplus
extern "C" {
#endif

int littlefs_mount();
int storage_init(void);

#ifdef __cplusplus
}
#endif

#endif
