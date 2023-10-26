#include <xss.h>
#include <zephyr/drivers/pm_cpu_ops.h>
#include <zephyr/kernel.h>

#include "reboot.h"

struct k_timer reboot_check_timer;
struct k_work  perform_reboot_work;

void perform_system_reboot(struct k_work* work)
{
    int                rc, value;
    static const char* path = CONFIG_AOS_REBOOT_XEN_STORE_PATH;

    rc = xss_read_integer(path, &value);
    if (rc) {
        if (rc != -ENOENT) {
            printk("Failed to read reboot value (%d)\n", rc);
        }

        return;
    }

    if (value != 2) {
        printk("Unexpected reboot value (%d)\n", value);
        return;
    }

    printk("Rebooting ....\n");

    rc = pm_system_reset(SYS_COLD_RESET);
    if (rc) {
        printk("Failed to reboot system (%d)\n", rc);
    }
}

static void trigger_reboot_check(struct k_timer* timer_id)
{
    k_work_submit(&perform_reboot_work);
}

void reboot_watcher_init()
{
    printk("Start guest reboot watcher\n");

    k_work_init(&perform_reboot_work, perform_system_reboot);
    k_timer_init(&reboot_check_timer, trigger_reboot_check, NULL);
    k_timer_start(&reboot_check_timer, K_SECONDS(CONFIG_AOS_REBOOT_CHECKING_PERIOD_SEC),
        K_SECONDS(CONFIG_AOS_REBOOT_CHECKING_PERIOD_SEC));
}
