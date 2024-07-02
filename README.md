[![ci](https://github.com/aosedge/aos_core_zephyr/actions/workflows/build_test.yaml/badge.svg)](https://github.com/aosedge/aos_core_zephyr/actions/workflows/build_test.yaml)
[![codecov](https://codecov.io/gh/aosedge/aos_core_zephyr/branch/main/graph/badge.svg?token=ObQrD8aaAC)](https://codecov.io/gh/aosedge/aos_core_zephyr)

# AosCore zephyr application

## Overview

This project contains code of the Aos core application for zephyr OS.

## Prerequisites

Zephyr SDK is required to fetch and build this project. Follow
[Getting Started Guide](https://docs.zephyrproject.org/latest/getting_started/index.html) to install mentioned
dependencies and SDK.

Install protobuf compiler from pre-compiled binaries: <https://grpc.io/docs/protoc-installation/#install-pre-compiled-binaries-any-os>.
The verified protobuf compiler version is v22.3: <https://github.com/protocolbuffers/protobuf/releases/tag/v22.3>.

## Fetch

Use zephyr `west` tool to fetch required repos:

```sh
west init -m  https://github.com/aosedge/aos_core_zephyr --mr main aos_zephyr_sdk
cd aos_zephyr_sdk
west update
west zephyr-export
```

## Build

```sh
cd aos_core_zephyr

west build -b ${BOARD} -p auto
west build -t run
```

For test and debug purpose `native_posix_64` or `native_posix` board can be used.

For real hardware with Xen the following command shall be used:

```sh
west build -b ${BOARD} -p auto -S xen_dom0 -- \
-D'DCONFIG_AOS_NODE_ID="my_node_id"' \
-D'DCONFIG_AOS_NODE_TYPE="my_node_type"' \
-D'CONFIG_DOMD_UBOOT_PATH="path/to/domd/u-boot"' \
-D'CONFIG_DOMD_DTB_PATH="path/to/domd/dtb"' \
-D'CONFIG_TA_DEPLOY_DIR="path/to/folder/with/ta's"'
-D'CONFIG_AOS_ROOT_CA_PATH="path/to/aos/root/cert"'
```

where:

* `CONFIG_AOS_NODE_ID` - Aos node ID;
* `CONFIG_AOS_NODE_TYPE` - Aos node type;
* `CONFIG_DOMD_UBOOT_PATH` - path to DomD u-boot binary;
* `CONFIG_DOMD_DTB_PATH` - path to DomD DTB file;
* `CONFIG_TA_DEPLOY_DIR` - path to directory with OPTEE TA's that shall to be build-in application;
* `CONFIG_AOS_ROOT_CA_PATH` - path to Aos root CA certificate.

Supported ${BOARD}: `rcar_spider`, `rcar_salvator_xs_m3` and `rcar_h3ulcb_ca57`

## Run

```sh
west build -t run
```

## Unit tests

Unit tests are implemented using zephyr [Test Framework](https://docs.zephyrproject.org/latest/develop/test/ztest.html).

Use the following commands to run the application unit tests:

```sh
west twister -c -v -T tests
```

All test reports will be saved in `twister-out` folder.

## Code coverage

Use the following command to calculate unit tests code coverage:

```sh
../zephyr/scripts/twister -c -v --coverage --coverage-basedir src/ --coverage-tool gcovr -p unit_testing -T tests
```

Open `twister-out/coverage/index.html` with your browser to see the code coverage result.

To see summary:

```sh
gcovr twister-out/unit_testing -f src/
```
