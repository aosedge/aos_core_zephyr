#ifndef REBOOT_H_
#define REBOOT_H_

#ifdef __cplusplus
extern "C" {
#endif

#define CONFIG_AOS_REBOOT_CHECKING_PERIOD_SEC 10

void reboot_watcher_init();

#ifdef __cplusplus
}
#endif

#endif
