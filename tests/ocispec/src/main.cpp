/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/tc_util.h>
#include <zephyr/ztest.h>

#include <ocispec/ocispec.hpp>

/***********************************************************************************************************************
 * Consts
 **********************************************************************************************************************/

static constexpr auto cImageSpecPath = "/tmp/aos/image.json";
static constexpr auto cRuntimeSpecPath = "/tmp/aos/runtime.json";

/***********************************************************************************************************************
 * Vars
 **********************************************************************************************************************/

OCISpec gOCISpecHelper;

/***********************************************************************************************************************
 * Tests
 **********************************************************************************************************************/

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

    auto err = gOCISpecHelper.SaveImageSpec(cImageSpecPath, writeImageSpec);
    zassert_true(err.IsNone(), "Cant save image spec: %s", err.Message());

    err = gOCISpecHelper.LoadImageSpec(cImageSpecPath, readImageSpec);
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

    writeRuntimeSpec.mOCIVersion = "1.0.0";

    writeVMConfig.mHypervisor.mPath = "HypPath";
    writeVMConfig.mHypervisor.mParameters.PushBack("HypParam1");
    writeVMConfig.mHypervisor.mParameters.PushBack("HypParam2");

    writeVMConfig.mKernel.mPath = "KerPath";
    writeVMConfig.mKernel.mParameters.PushBack("KerParam1");
    writeVMConfig.mKernel.mParameters.PushBack("KerParam2");
    writeVMConfig.mKernel.mParameters.PushBack("KerParam3");

    writeVMConfig.mHWConfig.mDeviceTree = "/path/to/device/tree";
    writeVMConfig.mHWConfig.mVCPUs = 42;
    writeVMConfig.mHWConfig.mMemKB = 8192;
    writeVMConfig.mHWConfig.mDTDevs.PushBack("dev1");
    writeVMConfig.mHWConfig.mDTDevs.PushBack("dev2");
    writeVMConfig.mHWConfig.mDTDevs.PushBack("dev3");
    writeVMConfig.mHWConfig.mIOMEMs.PushBack({1, 2, 3});
    writeVMConfig.mHWConfig.mIOMEMs.PushBack({4, 5, 6});
    writeVMConfig.mHWConfig.mIRQs.PushBack(1);
    writeVMConfig.mHWConfig.mIRQs.PushBack(2);
    writeVMConfig.mHWConfig.mIRQs.PushBack(3);
    writeVMConfig.mHWConfig.mIRQs.PushBack(4);

    auto ret = mkdir("/tmp/aos", S_IRWXU | S_IRWXG | S_IRWXO);
    if (ret != 0) {
        zassert_true(errno == EEXIST, "Can't create test folder");
    }

    auto err = gOCISpecHelper.SaveRuntimeSpec(cRuntimeSpecPath, writeRuntimeSpec);
    zassert_true(err.IsNone(), "Cant save runtime spec: %s", err.Message());

    err = gOCISpecHelper.LoadRuntimeSpec(cRuntimeSpecPath, readRuntimeSpec);
    zassert_true(err.IsNone(), "Cant load runtime spec: %s", err.Message());

    zassert_equal(writeRuntimeSpec.mOCIVersion, readRuntimeSpec.mOCIVersion, "Incorrect versions");

    zassert_equal(writeVMConfig.mHypervisor.mPath, readVMConfig.mHypervisor.mPath, "Incorrect hypervisor path");
    zassert_equal(writeVMConfig.mHypervisor.mParameters.Size(), readVMConfig.mHypervisor.mParameters.Size(),
        "Incorrect hypervisor parameters count");
    for (size_t i = 0; i < writeVMConfig.mHypervisor.mParameters.Size(); i++) {
        zassert_equal(writeVMConfig.mHypervisor.mParameters[i], readVMConfig.mHypervisor.mParameters[i],
            "Incorrect value hypervisor param");
    }

    zassert_equal(writeVMConfig.mKernel.mPath, readVMConfig.mKernel.mPath, "Incorrect kernel path");
    zassert_equal(writeVMConfig.mKernel.mParameters.Size(), readVMConfig.mKernel.mParameters.Size(),
        "Incorrect kernel parameters count");
    for (size_t i = 0; i < writeVMConfig.mKernel.mParameters.Size(); i++) {
        zassert_equal(
            writeVMConfig.mKernel.mParameters[i], readVMConfig.mKernel.mParameters[i], "Incorrect value kernel param");
    }

    zassert_equal(
        writeVMConfig.mHWConfig.mDeviceTree, readVMConfig.mHWConfig.mDeviceTree, "Incorrect device tree path");
    zassert_equal(writeVMConfig.mHWConfig.mVCPUs, readVMConfig.mHWConfig.mVCPUs, "Incorrect vCPUs value");
    zassert_equal(writeVMConfig.mHWConfig.mMemKB, readVMConfig.mHWConfig.mMemKB, "Incorrect memory value");
    zassert_equal(
        writeVMConfig.mHWConfig.mDTDevs.Size(), readVMConfig.mHWConfig.mDTDevs.Size(), "Incorrect DT devs count");
    for (size_t i = 0; i < writeVMConfig.mHWConfig.mDTDevs.Size(); i++) {
        zassert_equal(writeVMConfig.mHWConfig.mDTDevs[i], readVMConfig.mHWConfig.mDTDevs[i], "Incorrect DT dev");
    }
    zassert_equal(
        writeVMConfig.mHWConfig.mIOMEMs.Size(), readVMConfig.mHWConfig.mIOMEMs.Size(), "Incorrect IOMEM count");
    for (size_t i = 0; i < writeVMConfig.mHWConfig.mIOMEMs.Size(); i++) {
        zassert_equal(writeVMConfig.mHWConfig.mIOMEMs[i], readVMConfig.mHWConfig.mIOMEMs[i], "Incorrect IOMEM");
    }
    zassert_equal(writeVMConfig.mHWConfig.mIRQs.Size(), readVMConfig.mHWConfig.mIRQs.Size(), "Incorrect IRQ count");
    for (size_t i = 0; i < writeVMConfig.mHWConfig.mIRQs.Size(); i++) {
        zassert_equal(writeVMConfig.mHWConfig.mIRQs[i], readVMConfig.mHWConfig.mIRQs[i], "Incorrect IRQ");
    }
}
