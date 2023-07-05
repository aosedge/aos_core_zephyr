/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <fcntl.h>

#include <zephyr/data/json.h>

#include <aos/common/tools/memory.hpp>

#include "log.hpp"
#include "ocispec.hpp"

/***********************************************************************************************************************
 * Vars
 **********************************************************************************************************************/

// Image manifest

const struct json_obj_descr ContentDescriptorDescr[] = {
    JSON_OBJ_DESCR_PRIM(ContentDescriptor, mediaType, JSON_TOK_STRING),
    JSON_OBJ_DESCR_PRIM(ContentDescriptor, digest, JSON_TOK_STRING),
    JSON_OBJ_DESCR_PRIM(ContentDescriptor, size, JSON_TOK_NUMBER),
};

const struct json_obj_descr ImageManifestDescr[] = {
    JSON_OBJ_DESCR_PRIM(ImageManifest, schemaVersion, JSON_TOK_NUMBER),
    JSON_OBJ_DESCR_PRIM(ImageManifest, mediaType, JSON_TOK_STRING),
    JSON_OBJ_DESCR_OBJECT(ImageManifest, config, ContentDescriptorDescr),
    JSON_OBJ_DESCR_OBJ_ARRAY(ImageManifest, layers, aos::cMaxNumLayers, layersLen, ContentDescriptorDescr,
        ARRAY_SIZE(ContentDescriptorDescr)),
    JSON_OBJ_DESCR_OBJECT(ImageManifest, aosService, ContentDescriptorDescr),
};

// Image spec

const struct json_obj_descr ImageConfigDescr[] = {
    JSON_OBJ_DESCR_ARRAY(ImageConfig, Env, aos::oci::cMaxParamCount, envLen, JSON_TOK_STRING),
    JSON_OBJ_DESCR_ARRAY(ImageConfig, Entrypoint, aos::oci::cMaxParamCount, entrypointLen, JSON_TOK_STRING),
    JSON_OBJ_DESCR_ARRAY(ImageConfig, Cmd, aos::oci::cMaxParamCount, cmdLen, JSON_TOK_STRING),
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

static const struct json_obj_descr VMHWConfigIOMEMDescr[] = {
    JSON_OBJ_DESCR_PRIM(VMHWConfigIOMEM, firstGFN, JSON_TOK_NUMBER),
    JSON_OBJ_DESCR_PRIM(VMHWConfigIOMEM, firstMFN, JSON_TOK_NUMBER),
    JSON_OBJ_DESCR_PRIM(VMHWConfigIOMEM, nrMFNs, JSON_TOK_NUMBER),
};

static const struct json_obj_descr VMHWConfigDescr[] = {
    JSON_OBJ_DESCR_PRIM(VMHWConfig, deviceTree, JSON_TOK_STRING),
    JSON_OBJ_DESCR_PRIM(VMHWConfig, vcpus, JSON_TOK_NUMBER),
    JSON_OBJ_DESCR_PRIM(VMHWConfig, memKB, JSON_TOK_NUMBER),
    JSON_OBJ_DESCR_ARRAY(VMHWConfig, dtdevs, aos::oci::cMaxDTDevsCount, dtdevsLen, JSON_TOK_STRING),
    JSON_OBJ_DESCR_OBJ_ARRAY(VMHWConfig, iomems, aos::oci::cMaxIOMEMsCount, iomemsLen, VMHWConfigIOMEMDescr,
        ARRAY_SIZE(VMHWConfigIOMEMDescr)),
    JSON_OBJ_DESCR_ARRAY(VMHWConfig, irqs, aos::oci::cMaxIRQsCount, irqsLen, JSON_TOK_NUMBER),
};

static const struct json_obj_descr VMDescr[] = {
    JSON_OBJ_DESCR_OBJECT(VM, hypervisor, VMHypervisorDescr),
    JSON_OBJ_DESCR_OBJECT(VM, kernel, VMKernelDescr),
    JSON_OBJ_DESCR_OBJECT(VM, hwConfig, VMHWConfigDescr),
};

static const struct json_obj_descr RuntimeSpecDescr[] = {
    JSON_OBJ_DESCR_PRIM(RuntimeSpec, ociVersion, JSON_TOK_STRING),
    JSON_OBJ_DESCR_OBJECT(RuntimeSpec, vm, VMDescr),
};

/***********************************************************************************************************************
 * Public
 **********************************************************************************************************************/

aos::Error OCISpec::LoadImageManifest(const aos::String& path, aos::oci::ImageManifest& manifest)
{
    aos::LockGuard lock(mMutex);

    LOG_DBG() << "Load image manifest: " << path;

    auto readRet = ReadFileContentToBuffer(path);
    if (!readRet.mError.IsNone()) {
        return readRet.mError;
    }

    auto jsonImageManifest = aos::UniquePtr<ImageManifest>(&mAllocator, new (&mAllocator) ImageManifest);

    memset(jsonImageManifest.Get(), 0, sizeof(ImageManifest));

    jsonImageManifest->mediaType = "";
    jsonImageManifest->config.mediaType = "";
    jsonImageManifest->config.digest = "";
    jsonImageManifest->aosService.mediaType = "";
    jsonImageManifest->aosService.digest = "";

    int ret = json_obj_parse(static_cast<char*>(mJsonFileBuffer.Get()), readRet.mValue, ImageManifestDescr,
        ARRAY_SIZE(ImageManifestDescr), jsonImageManifest.Get());
    if (ret < 0) {
        return AOS_ERROR_WRAP(ret);
    }

    manifest.mSchemaVersion = jsonImageManifest->schemaVersion;
    manifest.mMediaType = jsonImageManifest->mediaType;

    // config

    manifest.mConfig = {jsonImageManifest->config.mediaType, jsonImageManifest->config.digest,
        static_cast<size_t>(jsonImageManifest->config.size)};

    // layers

    for (size_t i = 0; i < jsonImageManifest->layersLen; i++) {
        manifest.mLayers.PushBack({jsonImageManifest->layers[i].mediaType, jsonImageManifest->layers[i].digest,
            static_cast<size_t>(jsonImageManifest->layers[i].size)});
    }

    // aosService

    if (ret & eImageManifestAosServiceField) {
        if (manifest.mAosService == nullptr) {
            return AOS_ERROR_WRAP(aos::ErrorEnum::eNoMemory);
        }

        *manifest.mAosService = {jsonImageManifest->aosService.mediaType, jsonImageManifest->aosService.digest,
            static_cast<size_t>(jsonImageManifest->aosService.size)};
    }

    return aos::ErrorEnum::eNone;
}

aos::Error OCISpec::SaveImageManifest(const aos::String& path, const aos::oci::ImageManifest& manifest)
{
    aos::LockGuard lock(mMutex);

    LOG_DBG() << "Save image manifest: " << path;

    auto jsonImageManifest = aos::UniquePtr<ImageManifest>(&mAllocator, new (&mAllocator) ImageManifest);

    memset(jsonImageManifest.Get(), 0, sizeof(ImageManifest));

    jsonImageManifest->aosService.mediaType = "";
    jsonImageManifest->aosService.digest = "";

    jsonImageManifest->schemaVersion = manifest.mSchemaVersion;
    jsonImageManifest->mediaType = manifest.mMediaType.CStr();

    // config

    jsonImageManifest->config = {manifest.mConfig.mMediaType.CStr(), manifest.mConfig.mDigest.CStr(),
        static_cast<int64_t>(manifest.mConfig.mSize)};

    // layers

    jsonImageManifest->layersLen = manifest.mLayers.Size();

    for (size_t i = 0; i < jsonImageManifest->layersLen; i++) {
        jsonImageManifest->layers[i] = {manifest.mLayers[i].mMediaType.CStr(), manifest.mLayers[i].mDigest.CStr(),
            static_cast<int64_t>(manifest.mLayers[i].mSize)};
    }

    // aosService

    if (manifest.mAosService) {
        jsonImageManifest->aosService = {manifest.mAosService->mMediaType.CStr(), manifest.mAosService->mDigest.CStr(),
            static_cast<int64_t>(manifest.mAosService->mSize)};
    }

    json_obj_encode_buf(ImageManifestDescr, ARRAY_SIZE(ImageManifestDescr), jsonImageManifest.Get(),
        static_cast<char*>(mJsonFileBuffer.Get()), mJsonFileBuffer.Size());

    auto err = WriteEncodedJsonBufferToFile(path);
    if (!err.IsNone()) {
        return err;
    }

    return aos::ErrorEnum::eNone;
}

aos::Error OCISpec::LoadImageSpec(const aos::String& path, aos::oci::ImageSpec& imageSpec)
{
    aos::LockGuard lock(mMutex);

    LOG_DBG() << "Load image spec: " << path;

    auto readRet = ReadFileContentToBuffer(path);
    if (!readRet.mError.IsNone()) {
        return readRet.mError;
    }

    auto jsonImageSpec = aos::UniquePtr<ImageSpec>(&mAllocator, new (&mAllocator) ImageSpec);

    memset(jsonImageSpec.Get(), 0, sizeof(ImageSpec));

    int ret = json_obj_parse(static_cast<char*>(mJsonFileBuffer.Get()), readRet.mValue, ImageSpecDescr,
        ARRAY_SIZE(ImageSpecDescr), jsonImageSpec.Get());
    if (ret < 0) {
        return AOS_ERROR_WRAP(ret);
    }

    // mEnv

    for (size_t i = 0; i < jsonImageSpec->config.envLen; i++) {
        imageSpec.mConfig.mEnv.PushBack(jsonImageSpec->config.Env[i]);
    }

    // mEntryPoint

    for (size_t i = 0; i < jsonImageSpec->config.entrypointLen; i++) {
        imageSpec.mConfig.mEntryPoint.PushBack(jsonImageSpec->config.Entrypoint[i]);
    }

    // mCmd

    for (size_t i = 0; i < jsonImageSpec->config.cmdLen; i++) {
        imageSpec.mConfig.mCmd.PushBack(jsonImageSpec->config.Cmd[i]);
    }

    return aos::ErrorEnum::eNone;
}

aos::Error OCISpec::SaveImageSpec(const aos::String& path, const aos::oci::ImageSpec& imageSpec)
{
    aos::LockGuard lock(mMutex);

    LOG_DBG() << "Save image spec: " << path;

    auto jsonImageSpec = aos::UniquePtr<ImageSpec>(&mAllocator, new (&mAllocator) ImageSpec);

    memset(jsonImageSpec.Get(), 0, sizeof(ImageSpec));

    // mEnv

    jsonImageSpec->config.envLen = imageSpec.mConfig.mEnv.Size();

    for (size_t i = 0; i < imageSpec.mConfig.mEnv.Size(); i++) {
        jsonImageSpec->config.Env[i] = imageSpec.mConfig.mEnv[i].CStr();
    }

    // mEntryPoint

    jsonImageSpec->config.entrypointLen = imageSpec.mConfig.mEntryPoint.Size();

    for (size_t i = 0; i < imageSpec.mConfig.mEntryPoint.Size(); i++) {
        jsonImageSpec->config.Entrypoint[i] = imageSpec.mConfig.mEntryPoint[i].CStr();
    }

    // mCmd

    jsonImageSpec->config.cmdLen = imageSpec.mConfig.mCmd.Size();

    for (size_t i = 0; i < imageSpec.mConfig.mCmd.Size(); i++) {
        jsonImageSpec->config.Cmd[i] = imageSpec.mConfig.mCmd[i].CStr();
    }

    json_obj_encode_buf(ImageSpecDescr, ARRAY_SIZE(ImageSpecDescr), jsonImageSpec.Get(),
        static_cast<char*>(mJsonFileBuffer.Get()), mJsonFileBuffer.Size());

    auto err = WriteEncodedJsonBufferToFile(path);
    if (!err.IsNone()) {
        return err;
    }

    return aos::ErrorEnum::eNone;
}

aos::Error OCISpec::LoadRuntimeSpec(const aos::String& path, aos::oci::RuntimeSpec& runtimeSpec)
{
    aos::LockGuard lock(mMutex);

    LOG_DBG() << "Load runtime spec: " << path;

    auto readRet = ReadFileContentToBuffer(path);
    if (!readRet.mError.IsNone()) {
        return readRet.mError;
    }

    auto jsonRuntimeSpec = aos::UniquePtr<RuntimeSpec>(&mAllocator, new (&mAllocator) RuntimeSpec);

    memset(jsonRuntimeSpec.Get(), 0, sizeof(RuntimeSpec));

    jsonRuntimeSpec->ociVersion = "";
    jsonRuntimeSpec->vm.hypervisor.path = "";
    jsonRuntimeSpec->vm.kernel.path = "";
    jsonRuntimeSpec->vm.hwConfig.deviceTree = "";

    int ret = json_obj_parse(static_cast<char*>(mJsonFileBuffer.Get()), readRet.mValue, RuntimeSpecDescr,
        ARRAY_SIZE(RuntimeSpecDescr), jsonRuntimeSpec.Get());
    if (ret < 0) {
        return AOS_ERROR_WRAP(ret);
    }

    runtimeSpec.mOCIVersion = jsonRuntimeSpec->ociVersion;

    if (ret & eRuntimeVMField) {
        if (runtimeSpec.mVM == nullptr) {
            return AOS_ERROR_WRAP(aos::ErrorEnum::eNoMemory);
        }

        runtimeSpec.mVM->mHypervisor.mPath = jsonRuntimeSpec->vm.hypervisor.path;
        runtimeSpec.mVM->mKernel.mPath = jsonRuntimeSpec->vm.kernel.path;

        for (size_t i = 0; i < jsonRuntimeSpec->vm.hypervisor.parametersLen; i++) {
            runtimeSpec.mVM->mHypervisor.mParameters.PushBack(jsonRuntimeSpec->vm.hypervisor.parameters[i]);
        }

        for (size_t i = 0; i < jsonRuntimeSpec->vm.kernel.parametersLen; i++) {
            runtimeSpec.mVM->mKernel.mParameters.PushBack(jsonRuntimeSpec->vm.kernel.parameters[i]);
        }

        runtimeSpec.mVM->mHWConfig.mDeviceTree = jsonRuntimeSpec->vm.hwConfig.deviceTree;
        runtimeSpec.mVM->mHWConfig.mVCPUs = jsonRuntimeSpec->vm.hwConfig.vcpus;
        runtimeSpec.mVM->mHWConfig.mMemKB = jsonRuntimeSpec->vm.hwConfig.memKB;

        for (size_t i = 0; i < jsonRuntimeSpec->vm.hwConfig.dtdevsLen; i++) {
            runtimeSpec.mVM->mHWConfig.mDTDevs.PushBack(jsonRuntimeSpec->vm.hwConfig.dtdevs[i]);
        }

        for (size_t i = 0; i < jsonRuntimeSpec->vm.hwConfig.iomemsLen; i++) {
            auto iomem = jsonRuntimeSpec->vm.hwConfig.iomems[i];

            runtimeSpec.mVM->mHWConfig.mIOMEMs.PushBack({iomem.firstGFN, iomem.firstMFN, iomem.nrMFNs});
        }

        for (size_t i = 0; i < jsonRuntimeSpec->vm.hwConfig.irqsLen; i++) {
            runtimeSpec.mVM->mHWConfig.mIRQs.PushBack(jsonRuntimeSpec->vm.hwConfig.irqs[i]);
        }
    }

    return aos::ErrorEnum::eNone;
}

aos::Error OCISpec::SaveRuntimeSpec(const aos::String& path, const aos::oci::RuntimeSpec& runtimeSpec)
{
    aos::LockGuard lock(mMutex);

    LOG_DBG() << "Save runtime spec: " << path;

    auto jsonRuntimeSpec = aos::UniquePtr<RuntimeSpec>(&mAllocator, new (&mAllocator) RuntimeSpec);

    memset(jsonRuntimeSpec.Get(), 0, sizeof(RuntimeSpec));

    jsonRuntimeSpec->vm.hypervisor.path = "";
    jsonRuntimeSpec->vm.kernel.path = "";
    jsonRuntimeSpec->vm.hwConfig.deviceTree = "";
    jsonRuntimeSpec->ociVersion = runtimeSpec.mOCIVersion.CStr();

    if (runtimeSpec.mVM) {
        jsonRuntimeSpec->vm.hypervisor.path = const_cast<char*>(runtimeSpec.mVM->mHypervisor.mPath.CStr());
        jsonRuntimeSpec->vm.kernel.path = const_cast<char*>(runtimeSpec.mVM->mKernel.mPath.CStr());
        jsonRuntimeSpec->vm.hypervisor.parametersLen = runtimeSpec.mVM->mHypervisor.mParameters.Size();
        jsonRuntimeSpec->vm.kernel.parametersLen = runtimeSpec.mVM->mKernel.mParameters.Size();

        for (size_t i = 0; i < runtimeSpec.mVM->mHypervisor.mParameters.Size(); i++) {
            jsonRuntimeSpec->vm.hypervisor.parameters[i]
                = const_cast<char*>(runtimeSpec.mVM->mHypervisor.mParameters[i].CStr());
        }

        for (size_t i = 0; i < runtimeSpec.mVM->mKernel.mParameters.Size(); i++) {
            jsonRuntimeSpec->vm.kernel.parameters[i]
                = const_cast<char*>(runtimeSpec.mVM->mKernel.mParameters[i].CStr());
        }

        jsonRuntimeSpec->vm.hwConfig.deviceTree = runtimeSpec.mVM->mHWConfig.mDeviceTree.CStr();
        jsonRuntimeSpec->vm.hwConfig.vcpus = runtimeSpec.mVM->mHWConfig.mVCPUs;
        jsonRuntimeSpec->vm.hwConfig.memKB = runtimeSpec.mVM->mHWConfig.mMemKB;

        jsonRuntimeSpec->vm.hwConfig.dtdevsLen = runtimeSpec.mVM->mHWConfig.mDTDevs.Size();

        for (size_t i = 0; i < jsonRuntimeSpec->vm.hwConfig.dtdevsLen; i++) {
            jsonRuntimeSpec->vm.hwConfig.dtdevs[i] = runtimeSpec.mVM->mHWConfig.mDTDevs[i].CStr();
        }

        jsonRuntimeSpec->vm.hwConfig.iomemsLen = runtimeSpec.mVM->mHWConfig.mIOMEMs.Size();

        for (size_t i = 0; i < jsonRuntimeSpec->vm.hwConfig.iomemsLen; i++) {
            auto iomem = runtimeSpec.mVM->mHWConfig.mIOMEMs[i];

            jsonRuntimeSpec->vm.hwConfig.iomems[i] = {iomem.mFirstGFN, iomem.mFirstMFN, iomem.mNrMFNs};
        }

        jsonRuntimeSpec->vm.hwConfig.irqsLen = runtimeSpec.mVM->mHWConfig.mIRQs.Size();

        for (size_t i = 0; i < jsonRuntimeSpec->vm.hwConfig.irqsLen; i++) {
            jsonRuntimeSpec->vm.hwConfig.irqs[i] = runtimeSpec.mVM->mHWConfig.mIRQs[i];
        }
    }

    json_obj_encode_buf(RuntimeSpecDescr, ARRAY_SIZE(RuntimeSpecDescr), jsonRuntimeSpec.Get(),
        static_cast<char*>(mJsonFileBuffer.Get()), mJsonFileBuffer.Size());

    auto err = WriteEncodedJsonBufferToFile(path);
    if (!err.IsNone()) {
        return err;
    }

    return aos::ErrorEnum::eNone;
}

/***********************************************************************************************************************
 * Private
 **********************************************************************************************************************/

aos::RetWithError<size_t> OCISpec::ReadFileContentToBuffer(const aos::String& path)
{
    aos::RetWithError<size_t> err(0, aos::ErrorEnum::eNone);

    auto file = open(path.CStr(), O_RDONLY);
    if (file < 0) {
        err.mError = AOS_ERROR_WRAP(errno);
        return err;
    }

    err.mValue = read(file, mJsonFileBuffer.Get(), cJsonMaxContentSize);
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

aos::Error OCISpec::WriteEncodedJsonBufferToFile(const aos::String& path)
{
    aos::Error err = aos::ErrorEnum::eNone;

    // zephyr doesn't support O_TRUNC flag. This is WA to trunc file if it exists.
    auto ret = unlink(path.CStr());
    if (ret < 0 && errno != ENOENT) {
        return AOS_ERROR_WRAP(errno);
    }

    auto file = open(path.CStr(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if (file < 0) {
        return AOS_ERROR_WRAP(errno);
    }

    auto len = strlen(static_cast<char*>(mJsonFileBuffer.Get()));
    auto written = write(file, mJsonFileBuffer.Get(), len);
    if (written < 0) {
        err = AOS_ERROR_WRAP(errno);
    } else if ((size_t)written != len) {
        err = AOS_ERROR_WRAP(aos::ErrorEnum::eRuntime);
    }

    ret = close(file);
    if (err.IsNone() && (ret < 0)) {
        err = AOS_ERROR_WRAP(errno);
    }

    return err;
}
