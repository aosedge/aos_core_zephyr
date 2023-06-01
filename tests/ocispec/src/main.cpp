/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <fcntl.h>

#include <zephyr/tc_util.h>
#include <zephyr/ztest.h>

#include <ocispec/ocispec.hpp>

/***********************************************************************************************************************
 * Consts
 **********************************************************************************************************************/

static constexpr size_t cJsonMaxContentSize = 4096;

static constexpr auto cImageSpecPath = "/tmp/aos/image.json";
static constexpr auto cRuntimeSpecPath = "/tmp/aos/runtime.json";

/***********************************************************************************************************************
 * Private
 **********************************************************************************************************************/

static aos::RetWithError<size_t> ReadFile(const aos::String& path, const aos::Buffer& buffer)
{
    aos::RetWithError<size_t> result(0);

    auto file = open(path.CStr(), O_RDONLY);
    if (file < 0) {
        result.mError = {AOS_ERROR_WRAP(errno)};

        return result;
    }

    auto ret = read(file, buffer.Get(), buffer.Size());
    if (ret < 0) {
        result.mError = AOS_ERROR_WRAP(errno);
    }

    result.mValue = ret;

    ret = close(file);
    if (result.mError.IsNone() && ret < 0) {
        result.mError = AOS_ERROR_WRAP(errno);
    }

    return result;
}

aos::Error WriteFile(const aos::String& path, const void* data, size_t size)
{
    aos::Error err;

    // zephyr doesn't support O_TRUNC flag. This is WA to trunc file if it exists.
    auto ret = unlink(path.CStr());
    if (ret < 0 && errno != ENOENT) {
        return AOS_ERROR_WRAP(errno);
    }

    auto file = open(path.CStr(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if (file < 0) {
        return AOS_ERROR_WRAP(errno);
    }

    auto written = write(file, data, size);
    if (written < 0) {
        err = AOS_ERROR_WRAP(errno);
    } else if ((size_t)written != size) {
        err = AOS_ERROR_WRAP(aos::ErrorEnum::eRuntime);
    }

    ret = close(file);
    if (err.IsNone() && (ret < 0)) {
        err = AOS_ERROR_WRAP(errno);
    }

    return err;
}

/***********************************************************************************************************************
 * Setup
 **********************************************************************************************************************/

static void* Setup(void)
{
    zassert_true(aos::FS::MakeDirAll("/tmp/aos").IsNone(), "Can't create test folder");

    return nullptr;
}

static void Cleanup(void*)
{
    zassert_true(aos::FS::RemoveAll("/tmp/aos").IsNone(), "Can't remove test folder");
}

ZTEST_SUITE(ocispec, NULL, Setup, NULL, NULL, Cleanup);

/***********************************************************************************************************************
 * Tests
 **********************************************************************************************************************/

ZTEST(ocispec, ImageSpec)
{
    struct TestImageSpec {
        aos::oci::ImageSpec mImageSpec;
        char                mContent[cJsonMaxContentSize];
    };

    aos::StaticString<aos::oci::cMaxParamLen> envs[] = {"env0", "env1", "env2", "env3", "env4", "env5"};
    aos::StaticString<aos::oci::cMaxParamLen> entries[] = {"entry0", "entry1", "entry2"};
    aos::StaticString<aos::oci::cMaxParamLen> cmds[] = {"cmd0", "cmd1"};

    TestImageSpec testData[] = {
        // empty
        {
            {},
            R"({"config":{"Env":[],"Entrypoint":[],"Cmd":[]}})",
        },
        // Env
        {
            {
                aos::Array<aos::StaticString<aos::oci::cMaxParamLen>>(envs, aos::ArraySize(envs)),
            },
            R"({"config":{"Env":["env0","env1","env2","env3","env4","env5"],)"
            R"("Entrypoint":[],"Cmd":[]}})",
        },
        // Entrypoint
        {
            {
                aos::Array<aos::StaticString<aos::oci::cMaxParamLen>>(),
                aos::Array<aos::StaticString<aos::oci::cMaxParamLen>>(entries, aos::ArraySize(entries)),

            },
            R"({"config":{"Env":[],"Entrypoint":["entry0","entry1","entry2"],"Cmd":[]}})",
        },
        // Cmd
        {
            {
                aos::Array<aos::StaticString<aos::oci::cMaxParamLen>>(),
                aos::Array<aos::StaticString<aos::oci::cMaxParamLen>>(),
                aos::Array<aos::StaticString<aos::oci::cMaxParamLen>>(cmds, aos::ArraySize(cmds)),

            },
            R"({"config":{"Env":[],"Entrypoint":[],"Cmd":["cmd0","cmd1"]}})",
        },
        // All fields
        {
            {
                aos::Array<aos::StaticString<aos::oci::cMaxParamLen>>(envs, aos::ArraySize(envs)),
                aos::Array<aos::StaticString<aos::oci::cMaxParamLen>>(entries, aos::ArraySize(entries)),
                aos::Array<aos::StaticString<aos::oci::cMaxParamLen>>(cmds, aos::ArraySize(cmds)),

            },
            R"({"config":{"Env":["env0","env1","env2","env3","env4","env5"],)"
            R"("Entrypoint":["entry0","entry1","entry2"],"Cmd":["cmd0","cmd1"]}})",
        },
    };

    // Save image spec

    OCISpec ociSpec;

    for (auto testItem : testData) {
        auto err = ociSpec.SaveImageSpec(cImageSpecPath, testItem.mImageSpec);
        zassert_true(err.IsNone(), "Can't save image spec: %s", err.Message());

        aos::StaticBuffer<cJsonMaxContentSize> buffer;

        auto result = ReadFile(cImageSpecPath, buffer);
        zassert_true(result.mError.IsNone(), "Can't read image spec: %s", result.mError.Message());

        zassert_true(strncmp(testItem.mContent, static_cast<const char*>(buffer.Get()), result.mValue) == 0,
            "File content mismatch");
    }

    // Load image spec

    for (auto testItem : testData) {
        aos::oci::ImageSpec imageSpec {};

        auto err = WriteFile(cImageSpecPath, testItem.mContent, strlen(testItem.mContent));
        zassert_true(err.IsNone(), "Can't write spec file: %s", err.Message());

        err = ociSpec.LoadImageSpec(cImageSpecPath, imageSpec);
        zassert_true(err.IsNone(), "Can't load image spec: %s", err.Message());

        zassert_true(imageSpec == testItem.mImageSpec, "Image spec mismatch");
    }

    // Check file doesn't exist

    auto ret = unlink(cImageSpecPath);
    if (ret < 0 && errno != ENOENT) {
        zassert_true(true, "Can't remove spec file: %s", strerror(errno));
    }

    aos::oci::ImageSpec imageSpec;

    auto err = ociSpec.LoadImageSpec(cImageSpecPath, imageSpec);
    zassert_true(err.Is(ENOENT), "Wrong error: %s", err.Message());

    // Check empty file

    err = WriteFile(cImageSpecPath, "", 0);
    zassert_true(err.IsNone(), "Can't write spec file: %s", err.Message());

    err = ociSpec.LoadImageSpec(cImageSpecPath, imageSpec);
    zassert_true(err.Is(EINVAL), "Wrong error: %s", err.Message());

    // Check extra fields

    TestImageSpec extraFieldsData = {
        {
            aos::Array<aos::StaticString<aos::oci::cMaxParamLen>>(envs, aos::ArraySize(envs)),
            aos::Array<aos::StaticString<aos::oci::cMaxParamLen>>(entries, aos::ArraySize(entries)),
            aos::Array<aos::StaticString<aos::oci::cMaxParamLen>>(cmds, aos::ArraySize(cmds)),

        },
        R"({"config":{"Env":["env0","env1","env2","env3","env4","env5"],)"
        R"("Entrypoint":["entry0","entry1","entry2"],"Cmd":["cmd0","cmd1"],)"
        R"("extraField":123},"extraField":"test"})",
    };

    imageSpec = aos::oci::ImageSpec();

    err = WriteFile(cImageSpecPath, extraFieldsData.mContent, strlen(extraFieldsData.mContent));
    zassert_true(err.IsNone(), "Can't write spec file: %s", err.Message());

    err = ociSpec.LoadImageSpec(cImageSpecPath, imageSpec);
    zassert_true(err.IsNone(), "Can't load image spec: %s", err.Message());

    zassert_true(imageSpec == extraFieldsData.mImageSpec, "Image spec mismatch");
}

ZTEST(ocispec, RuntimeSpec)
{
    struct TestRuntimeSpec {
        aos::oci::RuntimeSpec mRuntimeSpec;
        char                  mContent[cJsonMaxContentSize];
    };

    aos::oci::VM vmEmpty;

    aos::StaticString<aos::oci::cMaxParamLen> hypervisorParams[] = {"hyp0", "hyp1", "hyp2"};
    aos::oci::VM                              vmWithHypervisor = {
        {
            "path0",
            aos::Array<aos::StaticString<aos::oci::cMaxParamLen>>(hypervisorParams, aos::ArraySize(hypervisorParams)),
        },
    };

    aos::StaticString<aos::oci::cMaxParamLen> kernelParams[] = {"krnl0", "krnl1"};
    aos::oci::VM                              vmWithKernel = {
        {},
        {
            "path1",
            aos::Array<aos::StaticString<aos::oci::cMaxParamLen>>(kernelParams, aos::ArraySize(kernelParams)),
        },
    };

    aos::StaticString<aos::oci::cMaxDTDevLen> dtdevs[] = {"dev0", "dev1", "dev2", "dev3"};
    aos::oci::VMHWConfigIOMEM                 iomems[] = {{0, 1, 2}, {3, 4, 5}};
    uint32_t                                  irqs[] = {1, 2, 3, 4, 5};
    aos::oci::VM                              vmWithHWConfig = {
        {},
        {},
        {
            "path2",
            3,
            5,
            aos::Array<aos::StaticString<aos::oci::cMaxDTDevLen>>(dtdevs, aos::ArraySize(dtdevs)),
            aos::Array<aos::oci::VMHWConfigIOMEM>(iomems, aos::ArraySize(iomems)),
            aos::Array<uint32_t>(irqs, aos::ArraySize(irqs)),
        },
    };

    aos::oci::VM vmWithAll = {
        {
            "path0",
            aos::Array<aos::StaticString<aos::oci::cMaxParamLen>>(hypervisorParams, aos::ArraySize(hypervisorParams)),
        },
        {
            "path1",
            aos::Array<aos::StaticString<aos::oci::cMaxParamLen>>(kernelParams, aos::ArraySize(kernelParams)),

        },
        {
            "path2",
            3,
            5,
            aos::Array<aos::StaticString<aos::oci::cMaxDTDevLen>>(dtdevs, aos::ArraySize(dtdevs)),
            aos::Array<aos::oci::VMHWConfigIOMEM>(iomems, aos::ArraySize(iomems)),
            aos::Array<uint32_t>(irqs, aos::ArraySize(irqs)),
        },
    };

    TestRuntimeSpec testData[] = {
        // empty
        {
            {"", &vmEmpty},
            R"({"ociVersion":"","vm":{)"
            R"("hypervisor":{"path":"","parameters":[]},)"
            R"("kernel":{"path":"","parameters":[]},)"
            R"("hwConfig":{"deviceTree":"","vcpus":0,"memKB":0,"dtdevs":[],"iomems":[],"irqs":[]}}})",
        },
        // ociVersion
        {
            {"1.0.0", &vmEmpty},
            R"({"ociVersion":"1.0.0","vm":{)"
            R"("hypervisor":{"path":"","parameters":[]},)"
            R"("kernel":{"path":"","parameters":[]},)"
            R"("hwConfig":{"deviceTree":"","vcpus":0,"memKB":0,"dtdevs":[],"iomems":[],"irqs":[]}}})",
        },
        // hypervisor
        {
            {"", &vmWithHypervisor},
            R"({"ociVersion":"","vm":{)"
            R"("hypervisor":{"path":"path0","parameters":["hyp0","hyp1","hyp2"]},)"
            R"("kernel":{"path":"","parameters":[]},)"
            R"("hwConfig":{"deviceTree":"","vcpus":0,"memKB":0,"dtdevs":[],"iomems":[],"irqs":[]}}})",
        },
        // kernel
        {
            {"", &vmWithKernel},
            R"({"ociVersion":"","vm":{)"
            R"("hypervisor":{"path":"","parameters":[]},)"
            R"("kernel":{"path":"path1","parameters":["krnl0","krnl1"]},)"
            R"("hwConfig":{"deviceTree":"","vcpus":0,"memKB":0,"dtdevs":[],"iomems":[],"irqs":[]}}})",
        },
        // hwConfig
        {
            {"", &vmWithHWConfig},
            R"({"ociVersion":"","vm":{)"
            R"("hypervisor":{"path":"","parameters":[]},)"
            R"("kernel":{"path":"","parameters":[]},)"
            R"("hwConfig":{"deviceTree":"path2","vcpus":3,"memKB":5,)"
            R"("dtdevs":["dev0","dev1","dev2","dev3"],)"
            R"("iomems":[{"firstGFN":0,"firstMFN":1,"nrMFNs":2},{"firstGFN":3,"firstMFN":4,"nrMFNs":5}],)"
            R"("irqs":[1,2,3,4,5]}}})",
        },
        // All fields
        {
            {"1.0.0", &vmWithAll},
            R"({"ociVersion":"1.0.0","vm":{)"
            R"("hypervisor":{"path":"path0","parameters":["hyp0","hyp1","hyp2"]},)"
            R"("kernel":{"path":"path1","parameters":["krnl0","krnl1"]},)"
            R"("hwConfig":{"deviceTree":"path2","vcpus":3,"memKB":5,)"
            R"("dtdevs":["dev0","dev1","dev2","dev3"],)"
            R"("iomems":[{"firstGFN":0,"firstMFN":1,"nrMFNs":2},{"firstGFN":3,"firstMFN":4,"nrMFNs":5}],)"
            R"("irqs":[1,2,3,4,5]}}})",
        },
    };

    // Save runtime spec

    OCISpec ociSpec;

    for (auto testItem : testData) {
        auto err = ociSpec.SaveRuntimeSpec(cRuntimeSpecPath, testItem.mRuntimeSpec);
        zassert_true(err.IsNone(), "Can't save runtime spec: %s", err.Message());

        aos::StaticBuffer<cJsonMaxContentSize> buffer;

        auto result = ReadFile(cRuntimeSpecPath, buffer);
        zassert_true(result.mError.IsNone(), "Can't read runtime spec: %s", result.mError.Message());

        zassert_true(strncmp(testItem.mContent, static_cast<const char*>(buffer.Get()), result.mValue) == 0,
            "File content mismatch");
    }

    // Load runtime spec

    for (auto testItem : testData) {
        aos::oci::VM          vmSpec;
        aos::oci::RuntimeSpec runtimeSpec {"", &vmSpec};

        auto err = WriteFile(cRuntimeSpecPath, testItem.mContent, strlen(testItem.mContent));
        zassert_true(err.IsNone(), "Can't write runtime file: %s", err.Message());

        err = ociSpec.LoadRuntimeSpec(cRuntimeSpecPath, runtimeSpec);
        zassert_true(err.IsNone(), "Can't load runtime spec: %s", err.Message());

        zassert_true(runtimeSpec == testItem.mRuntimeSpec, "Runtime spec mismatch");
    }

    // Check file doesn't exist

    auto ret = unlink(cRuntimeSpecPath);
    if (ret < 0 && errno != ENOENT) {
        zassert_true(true, "Can't remove spec file: %s", strerror(errno));
    }

    aos::oci::RuntimeSpec runtimeSpec {};

    auto err = ociSpec.LoadRuntimeSpec(cRuntimeSpecPath, runtimeSpec);
    zassert_true(err.Is(ENOENT), "Wrong error: %s", err.Message());

    // Check empty file

    err = WriteFile(cRuntimeSpecPath, "", 0);
    zassert_true(err.IsNone(), "Can't write spec file: %s", err.Message());

    err = ociSpec.LoadRuntimeSpec(cRuntimeSpecPath, runtimeSpec);
    zassert_true(err.Is(EINVAL), "Wrong error: %d", err.Errno());

    // VM spec without VM field

    err = WriteFile(cRuntimeSpecPath, testData[5].mContent, strlen(testData[5].mContent));
    zassert_true(err.IsNone(), "Can't write spec file: %s", err.Message());

    err = ociSpec.LoadRuntimeSpec(cRuntimeSpecPath, runtimeSpec);
    zassert_true(err.Is(aos::ErrorEnum::eNoMemory), "Wrong error: %s", err.Message());

    // Check extra fields

    TestRuntimeSpec extraFieldsData = {
        {"1.0.0", &vmWithAll},
        R"({"ociVersion":"1.0.0","vm":{)"
        R"("hypervisor":{"path":"path0","parameters":["hyp0","hyp1","hyp2"],"extraHypervisor":"1234"},)"
        R"("kernel":{"path":"path1","parameters":["krnl0","krnl1"],"extraKernel":"1234"},)"
        R"("hwConfig":{"deviceTree":"path2","vcpus":3,"memKB":5,)"
        R"("dtdevs":["dev0","dev1","dev2","dev3"],)"
        R"("iomems":[{"firstGFN":0,"firstMFN":1,"nrMFNs":2},{"firstGFN":3,"firstMFN":4,"nrMFNs":5}],)"
        R"("irqs":[1,2,3,4,5]},"extraHWConfig":"1234"},)"
        R"("extraField":1234})",
    };

    aos::oci::VM vmSpec;

    runtimeSpec.mVM = &vmSpec;

    err = WriteFile(cRuntimeSpecPath, extraFieldsData.mContent, strlen(extraFieldsData.mContent));
    zassert_true(err.IsNone(), "Can't write spec file: %s", err.Message());

    err = ociSpec.LoadRuntimeSpec(cRuntimeSpecPath, runtimeSpec);
    zassert_true(err.IsNone(), "Can't load runtime spec: %s", err.Message());

    zassert_true(runtimeSpec == extraFieldsData.mRuntimeSpec, "Runtime spec mismatch");
}
