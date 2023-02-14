# Aos Core Zephyr project

---
## Overview
This project contains code of the Aos Core application, which is intended to provide SOTA/FOTA updates.

---
## Build
To fetch and build this project few steps are required.

First of all you need to pass Zephyr RTOS [getting started guide](https://docs.zephyrproject.org/latest/getting_started/index.html) and install mentioned dependencies and SDK.

Follow commands below to fetch, build and run zephyr under Xen hypervisor in emulated Cortex A53:

```
$: west init -m  https://github.com/aoscloud/aos_core_zephyr --mr main aos_core_app
$: cd aos_core_app
$: west update
$: west zephyr-export
$: cd aos_core_zephyr
$: west build -b xenvm-qemu -p always
$: west build -t run
```
