/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/fs/fs.h>

#include <fcntl.h>
#include <unistd.h>

#include "ocispec.hpp"

aos::Error OCISpec::LoadImageSpec(const aos::String& path, aos::oci::ImageSpec& imageSpec)
{
    aos::LockGuard lock(mBufferMutex);

    auto readRet = ReadFileContentToBuffer(path, cJsonMaxContentSize);
    if (!readRet.mError.IsNone()) {
        return readRet.mError;
    }

    int ret = json_obj_parse(
        (char*)mJsonFileBuffer.Get(), readRet.mValue, ImageSpecJsonDescr, ARRAY_SIZE(ImageSpecJsonDescr), &mImageSpec);
    if (ret < 0) {
        return AOS_ERROR_WRAP(ret);
    }

    for (uint8_t i = 0; i < mImageSpec.config.cmdLen; i++) {
        imageSpec.mConfig.mCmd.PushBack(mImageSpec.config.Cmd[i]);
    }

    for (uint8_t i = 0; i < mImageSpec.config.entrypointLen; i++) {
        imageSpec.mConfig.mEntryPoint.PushBack(mImageSpec.config.entrypoint[i]);
    }

    return aos::ErrorEnum::eNone;
}

aos::Error OCISpec::SaveImageSpec(const aos::String& path, const aos::oci::ImageSpec& imageSpec)
{
    aos::LockGuard lock(mBufferMutex);

    mImageSpec.config.cmdLen = imageSpec.mConfig.mCmd.Size();
    mImageSpec.config.entrypointLen = imageSpec.mConfig.mEntryPoint.Size();

    for (size_t i = 0; i < imageSpec.mConfig.mCmd.Size(); i++) {
        mImageSpec.config.Cmd[i] = const_cast<char*>(imageSpec.mConfig.mCmd[i].CStr());
    }

    for (size_t i = 0; i < imageSpec.mConfig.mEntryPoint.Size(); i++) {
        mImageSpec.config.entrypoint[i] = const_cast<char*>(imageSpec.mConfig.mEntryPoint[i].CStr());
    }

    json_obj_encode_buf(ImageSpecJsonDescr, ARRAY_SIZE(ImageSpecJsonDescr), &mImageSpec, (char*)mJsonFileBuffer.Get(),
        cJsonMaxContentSize);

    auto err = WriteEncodedJsonBufferToFile(path, strlen((char*)mJsonFileBuffer.Get()));
    if (!err.IsNone()) {
        return err;
    }

    return aos::ErrorEnum::eNone;
}

aos::Error OCISpec::LoadRuntimeSpec(const aos::String& path, aos::oci::RuntimeSpec& runtimeSpec)
{
    if (runtimeSpec.mVM == nullptr) {
        return AOS_ERROR_WRAP(aos::ErrorEnum::eInvalidArgument);
    }

    aos::LockGuard lock(mBufferMutex);

    auto readRet = ReadFileContentToBuffer(path, cJsonMaxContentSize);
    if (!readRet.mError.IsNone()) {
        return readRet.mError;
    }

    int ret = json_obj_parse((char*)mJsonFileBuffer.Get(), readRet.mValue, RuntimeSpecJsonDescr,
        ARRAY_SIZE(RuntimeSpecJsonDescr), &mRuntimeSpec);
    if (ret < 0) {
        return AOS_ERROR_WRAP(ret);
    }

    runtimeSpec.mVersion = mRuntimeSpec.ociVersion;
    runtimeSpec.mVM->mHypervisor.mPath = mRuntimeSpec.vm.hypervisor.path;
    runtimeSpec.mVM->mKernel.mPath = mRuntimeSpec.vm.kernel.path;

    for (auto i = 0; i < mRuntimeSpec.vm.hypervisor.paramsLen; i++) {
        runtimeSpec.mVM->mHypervisor.mParameters.PushBack(mRuntimeSpec.vm.hypervisor.parameters[i]);
    }

    for (auto i = 0; i < mRuntimeSpec.vm.kernel.paramsLen; i++) {
        runtimeSpec.mVM->mKernel.mParameters.PushBack(mRuntimeSpec.vm.kernel.parameters[i]);
    }

    return aos::ErrorEnum::eNone;
}

aos::Error OCISpec::SaveRuntimeSpec(const aos::String& path, const aos::oci::RuntimeSpec& runtimeSpec)
{
    if (runtimeSpec.mVM == nullptr) {
        return AOS_ERROR_WRAP(aos::ErrorEnum::eInvalidArgument);
    }

    aos::LockGuard lock(mBufferMutex);

    mRuntimeSpec.ociVersion = const_cast<char*>(runtimeSpec.mVersion.CStr());
    mRuntimeSpec.vm.hypervisor.path = const_cast<char*>(runtimeSpec.mVM->mHypervisor.mPath.CStr());
    mRuntimeSpec.vm.kernel.path = const_cast<char*>(runtimeSpec.mVM->mKernel.mPath.CStr());
    mRuntimeSpec.vm.hypervisor.paramsLen = runtimeSpec.mVM->mHypervisor.mParameters.Size();
    mRuntimeSpec.vm.kernel.paramsLen = runtimeSpec.mVM->mKernel.mParameters.Size();

    for (size_t i = 0; i < runtimeSpec.mVM->mHypervisor.mParameters.Size(); i++) {
        mRuntimeSpec.vm.hypervisor.parameters[i]
            = const_cast<char*>(runtimeSpec.mVM->mHypervisor.mParameters[i].CStr());
    }

    for (size_t i = 0; i < runtimeSpec.mVM->mKernel.mParameters.Size(); i++) {
        mRuntimeSpec.vm.kernel.parameters[i] = const_cast<char*>(runtimeSpec.mVM->mKernel.mParameters[i].CStr());
    }

    mRuntimeSpec.vm.hwconfig.devicetree = const_cast<char*>("");

    json_obj_encode_buf(RuntimeSpecJsonDescr, ARRAY_SIZE(RuntimeSpecJsonDescr), &mRuntimeSpec,
        (char*)mJsonFileBuffer.Get(), cJsonMaxContentSize);

    auto err = WriteEncodedJsonBufferToFile(path, strlen((char*)mJsonFileBuffer.Get()));
    if (!err.IsNone()) {
        return err;
    }

    return aos::ErrorEnum::eNone;
}

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
