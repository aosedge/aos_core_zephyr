/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <fcntl.h>

#include <zephyr/data/json.h>

#include "ocispec.hpp"

/***********************************************************************************************************************
 * Vars
 **********************************************************************************************************************/

// Image spec

const struct json_obj_descr ImageConfigDescr[] = {
    JSON_OBJ_DESCR_ARRAY(ImageConfig, Cmd, aos::oci::cMaxParamCount, cmdLen, JSON_TOK_STRING),
    JSON_OBJ_DESCR_ARRAY(ImageConfig, Env, aos::oci::cMaxParamCount, envLen, JSON_TOK_STRING),
    JSON_OBJ_DESCR_ARRAY(ImageConfig, entrypoint, aos::oci::cMaxParamCount, entrypointLen, JSON_TOK_STRING),
};

const struct json_obj_descr ImageSpecDescr[] = {
    JSON_OBJ_DESCR_OBJECT(ImageSpec, config, ImageConfigDescr),
};

// Runtime spec

static const struct json_obj_descr VMHypervisorDescr[] = {
    JSON_OBJ_DESCR_PRIM(VMHypervisor, path, JSON_TOK_STRING),
    JSON_OBJ_DESCR_ARRAY(VMHypervisor, parameters, aos::oci::cMaxParamCount, parametersLen, JSON_TOK_STRING),
};

static const struct json_obj_descr VMKernelDescr[] = {
    JSON_OBJ_DESCR_PRIM(VMKernel, path, JSON_TOK_STRING),
    JSON_OBJ_DESCR_ARRAY(VMKernel, parameters, aos::oci::cMaxParamCount, parametersLen, JSON_TOK_STRING),
};

static const struct json_obj_descr VMHWConfigDescr[] = {
    JSON_OBJ_DESCR_PRIM(VMHWConfig, devicetree, JSON_TOK_STRING),
};

static const struct json_obj_descr VMDescr[] = {
    JSON_OBJ_DESCR_OBJECT(VM, hypervisor, VMHypervisorDescr),
    JSON_OBJ_DESCR_OBJECT(VM, kernel, VMKernelDescr),
    JSON_OBJ_DESCR_OBJECT(VM, hwconfig, VMHWConfigDescr),
};

static const struct json_obj_descr RuntimeSpecDescr[] = {
    JSON_OBJ_DESCR_PRIM(RuntimeSpec, ociVersion, JSON_TOK_STRING),
    JSON_OBJ_DESCR_OBJECT(RuntimeSpec, vm, VMDescr),
};

/***********************************************************************************************************************
 * Public
 **********************************************************************************************************************/

aos::Error OCISpec::LoadImageSpec(const aos::String& path, aos::oci::ImageSpec& imageSpec)
{
    aos::LockGuard lock(mMutex);

    auto readRet = ReadFileContentToBuffer(path, cJsonMaxContentSize);
    if (!readRet.mError.IsNone()) {
        return readRet.mError;
    }

    int ret = json_obj_parse(static_cast<char*>(mJsonFileBuffer.Get()), readRet.mValue, ImageSpecDescr,
        ARRAY_SIZE(ImageSpecDescr), &mImageSpec);
    if (ret < 0) {
        return AOS_ERROR_WRAP(ret);
    }

    for (size_t i = 0; i < mImageSpec.config.cmdLen; i++) {
        imageSpec.mConfig.mCmd.PushBack(mImageSpec.config.Cmd[i]);
    }

    for (size_t i = 0; i < mImageSpec.config.entrypointLen; i++) {
        imageSpec.mConfig.mEntryPoint.PushBack(mImageSpec.config.entrypoint[i]);
    }

    return aos::ErrorEnum::eNone;
}

aos::Error OCISpec::SaveImageSpec(const aos::String& path, const aos::oci::ImageSpec& imageSpec)
{
    aos::LockGuard lock(mMutex);

    mImageSpec.config.cmdLen = imageSpec.mConfig.mCmd.Size();
    mImageSpec.config.entrypointLen = imageSpec.mConfig.mEntryPoint.Size();

    for (size_t i = 0; i < imageSpec.mConfig.mCmd.Size(); i++) {
        mImageSpec.config.Cmd[i] = imageSpec.mConfig.mCmd[i].CStr();
    }

    for (size_t i = 0; i < imageSpec.mConfig.mEntryPoint.Size(); i++) {
        mImageSpec.config.entrypoint[i] = imageSpec.mConfig.mEntryPoint[i].CStr();
    }

    json_obj_encode_buf(ImageSpecDescr, ARRAY_SIZE(ImageSpecDescr), &mImageSpec,
        static_cast<char*>(mJsonFileBuffer.Get()), cJsonMaxContentSize);

    auto err = WriteEncodedJsonBufferToFile(path, strlen(static_cast<char*>(mJsonFileBuffer.Get())));
    if (!err.IsNone()) {
        return err;
    }

    return aos::ErrorEnum::eNone;
}

aos::Error OCISpec::LoadRuntimeSpec(const aos::String& path, aos::oci::RuntimeSpec& runtimeSpec)
{
    aos::LockGuard lock(mMutex);

    if (runtimeSpec.mVM == nullptr) {
        return AOS_ERROR_WRAP(aos::ErrorEnum::eInvalidArgument);
    }

    auto readRet = ReadFileContentToBuffer(path, cJsonMaxContentSize);
    if (!readRet.mError.IsNone()) {
        return readRet.mError;
    }

    int ret = json_obj_parse(static_cast<char*>(mJsonFileBuffer.Get()), readRet.mValue, RuntimeSpecDescr,
        ARRAY_SIZE(RuntimeSpecDescr), &mRuntimeSpec);
    if (ret < 0) {
        return AOS_ERROR_WRAP(ret);
    }

    runtimeSpec.mVersion = mRuntimeSpec.ociVersion;
    runtimeSpec.mVM->mHypervisor.mPath = mRuntimeSpec.vm.hypervisor.path;
    runtimeSpec.mVM->mKernel.mPath = mRuntimeSpec.vm.kernel.path;

    for (size_t i = 0; i < mRuntimeSpec.vm.hypervisor.parametersLen; i++) {
        runtimeSpec.mVM->mHypervisor.mParameters.PushBack(mRuntimeSpec.vm.hypervisor.parameters[i]);
    }

    for (size_t i = 0; i < mRuntimeSpec.vm.kernel.parametersLen; i++) {
        runtimeSpec.mVM->mKernel.mParameters.PushBack(mRuntimeSpec.vm.kernel.parameters[i]);
    }

    return aos::ErrorEnum::eNone;
}

aos::Error OCISpec::SaveRuntimeSpec(const aos::String& path, const aos::oci::RuntimeSpec& runtimeSpec)
{
    aos::LockGuard lock(mMutex);

    if (runtimeSpec.mVM == nullptr) {
        return AOS_ERROR_WRAP(aos::ErrorEnum::eInvalidArgument);
    }

    mRuntimeSpec.ociVersion = runtimeSpec.mVersion.CStr();
    mRuntimeSpec.vm.hypervisor.path = runtimeSpec.mVM->mHypervisor.mPath.CStr();
    mRuntimeSpec.vm.kernel.path = runtimeSpec.mVM->mKernel.mPath.CStr();
    mRuntimeSpec.vm.hypervisor.parametersLen = runtimeSpec.mVM->mHypervisor.mParameters.Size();
    mRuntimeSpec.vm.kernel.parametersLen = runtimeSpec.mVM->mKernel.mParameters.Size();

    for (size_t i = 0; i < runtimeSpec.mVM->mHypervisor.mParameters.Size(); i++) {
        mRuntimeSpec.vm.hypervisor.parameters[i] = runtimeSpec.mVM->mHypervisor.mParameters[i].CStr();
    }

    for (size_t i = 0; i < runtimeSpec.mVM->mKernel.mParameters.Size(); i++) {
        mRuntimeSpec.vm.kernel.parameters[i] = runtimeSpec.mVM->mKernel.mParameters[i].CStr();
    }

    mRuntimeSpec.vm.hwconfig.devicetree = "";

    json_obj_encode_buf(RuntimeSpecDescr, ARRAY_SIZE(RuntimeSpecDescr), &mRuntimeSpec,
        static_cast<char*>(mJsonFileBuffer.Get()), cJsonMaxContentSize);

    auto err = WriteEncodedJsonBufferToFile(path, strlen(static_cast<char*>(mJsonFileBuffer.Get())));
    if (!err.IsNone()) {
        return err;
    }

    return aos::ErrorEnum::eNone;
}

/***********************************************************************************************************************
 * Private
 **********************************************************************************************************************/

aos::RetWithError<size_t> OCISpec::ReadFileContentToBuffer(const aos::String& path, size_t maxContentSize)
{
    aos::RetWithError<size_t> err(0, aos::ErrorEnum::eNone);

    auto file = open(path.CStr(), O_RDONLY);
    if (file < 0) {
        err.mError = AOS_ERROR_WRAP(errno);
        return err;
    }

    err.mValue = read(file, mJsonFileBuffer.Get(), maxContentSize);
    if (err.mValue < 0) {
        err.mError = AOS_ERROR_WRAP(errno);
    }

    auto ret = close(file);
    if (err.mError.IsNone() && (ret < 0)) {
        err.mError = AOS_ERROR_WRAP(errno);
        err.mValue = ret;
    }

    return err;
}

aos::Error OCISpec::WriteEncodedJsonBufferToFile(const aos::String& path, size_t len)
{
    aos::Error err = aos::ErrorEnum::eNone;

    auto file = open(path.CStr(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if (file < 0) {
        return AOS_ERROR_WRAP(errno);
    }

    auto written = write(file, mJsonFileBuffer.Get(), len);
    if (written < 0) {
        err = AOS_ERROR_WRAP(errno);
    } else if ((size_t)written != len) {
        err = AOS_ERROR_WRAP(aos::ErrorEnum::eRuntime);
    }

    auto ret = close(file);
    if (err.IsNone() && (ret < 0)) {
        err = AOS_ERROR_WRAP(errno);
    }

    return err;
}
