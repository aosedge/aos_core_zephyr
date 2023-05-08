/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/tc_util.h>
#include <zephyr/ztest.h>

#include <ocispec/ocispec.hpp>

OCISpec gOCISpecHelper;

ZTEST_SUITE(ocispec, NULL, NULL, NULL, NULL, NULL);

ZTEST(ocispec, test_read_write_ImageSpec)
{
    aos::oci::ImageSpec writeImageSpec, readImageSpec;

    writeImageSpec.mConfig.mCmd.PushBack("/bin/sh");
    writeImageSpec.mConfig.mCmd.PushBack("hello_world.sh");
    writeImageSpec.mConfig.mEntryPoint.PushBack("Entry1");

    auto ret = mkdir("/tmp/aos", S_IRWXU | S_IRWXG | S_IRWXO);
    if (ret != 0) {
        zassert_true(errno == EEXIST, "Can't create test folder");
    }

    auto err = gOCISpecHelper.SaveImageSpec("/tmp/aos/ImageSpec.json", writeImageSpec);
    zassert_true(err.IsNone(), "Cant save image spec: %s", err.Message());

    err = gOCISpecHelper.LoadImageSpec("/tmp/aos/ImageSpec.json", readImageSpec);
    zassert_true(err.IsNone(), "Cant load image spec: %s", err.Message());

    for (size_t i = 0; i < writeImageSpec.mConfig.mCmd.Size(); i++) {
        zassert_equal(writeImageSpec.mConfig.mCmd[i], readImageSpec.mConfig.mCmd[i], "Incorrect value for cmd");
    }

    for (size_t i = 0; i < writeImageSpec.mConfig.mEntryPoint.Size(); i++) {
        zassert_equal(writeImageSpec.mConfig.mEntryPoint[i], readImageSpec.mConfig.mEntryPoint[i],
            "Incorrect value for entryPoint");
    }
}

ZTEST(ocispec, test_read_write_RuntimeSpec)
{
    aos::oci::RuntimeSpec writeRuntimeSpec, readRuntimeSpec;
    aos::oci::VM          writeVMConfig, readVMConfig;

    writeRuntimeSpec.mVM = &writeVMConfig;
    readRuntimeSpec.mVM = &readVMConfig;

    writeRuntimeSpec.mVersion = "1.0.0";
    writeVMConfig.mHypervisor.mPath = "HypPath";
    writeVMConfig.mHypervisor.mParameters.PushBack("HypParam1");
    writeVMConfig.mHypervisor.mParameters.PushBack("HypParam2");

    writeVMConfig.mKernel.mPath = "KerPath";
    writeVMConfig.mKernel.mParameters.PushBack("KerParam1");
    writeVMConfig.mKernel.mParameters.PushBack("KerParam2");
    writeVMConfig.mKernel.mParameters.PushBack("KerParam3");

    auto ret = mkdir("/tmp/aos", S_IRWXU | S_IRWXG | S_IRWXO);
    if (ret != 0) {
        zassert_true(errno == EEXIST, "Can't create test folder");
    }

    auto err = gOCISpecHelper.SaveRuntimeSpec("/tmp/aos/RuntimeSpec.json", writeRuntimeSpec);
    zassert_true(err.IsNone(), "Cant save runtime spec: %s", err.Message());

    err = gOCISpecHelper.LoadRuntimeSpec("/tmp/aos/RuntimeSpec.json", readRuntimeSpec);
    zassert_true(err.IsNone(), "Cant load runtime spec: %s", err.Message());

    zassert_equal(writeRuntimeSpec.mVersion, readRuntimeSpec.mVersion, "Incorrect versions");
    zassert_equal(writeVMConfig.mHypervisor.mPath, readVMConfig.mHypervisor.mPath, "Incorrect hypervisor path");
    zassert_equal(writeVMConfig.mKernel.mPath, readVMConfig.mKernel.mPath, "Incorrect kernel path");

    for (size_t i = 0; i < writeVMConfig.mHypervisor.mParameters.Size(); i++) {
        zassert_equal(writeVMConfig.mHypervisor.mParameters[i], readVMConfig.mHypervisor.mParameters[i],
            "Incorrect value hypervisor param");
    }

    for (size_t i = 0; i < writeVMConfig.mKernel.mParameters.Size(); i++) {
        zassert_equal(
            writeVMConfig.mKernel.mParameters[i], readVMConfig.mKernel.mParameters[i], "Incorrect value kernel param");
    }
}
