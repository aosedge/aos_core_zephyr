/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <fcntl.h>

#include <aos/common/tools/fs.hpp>
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
    JSON_OBJ_DESCR_PRIM(ContentDescriptor, size, JSON_TOK_FLOAT),
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
    JSON_OBJ_DESCR_PRIM(VMHWConfigIOMEM, firstGFN, JSON_TOK_FLOAT),
    JSON_OBJ_DESCR_PRIM(VMHWConfigIOMEM, firstMFN, JSON_TOK_FLOAT),
    JSON_OBJ_DESCR_PRIM(VMHWConfigIOMEM, nrMFNs, JSON_TOK_FLOAT),
};

static const struct json_obj_descr VMHWConfigDescr[] = {
    JSON_OBJ_DESCR_PRIM(VMHWConfig, deviceTree, JSON_TOK_STRING),
    JSON_OBJ_DESCR_PRIM(VMHWConfig, vcpus, JSON_TOK_NUMBER),
    JSON_OBJ_DESCR_PRIM(VMHWConfig, memKB, JSON_TOK_FLOAT),
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

// cppcheck-suppress unusedFunction
aos::Error OCISpec::LoadImageManifest(const aos::String& path, aos::oci::ImageManifest& manifest)
{
    aos::LockGuard lock(mMutex);

    LOG_DBG() << "Load image manifest: " << path;

    auto err = aos::FS::ReadFileToString(path, mJsonFileContent);
    if (!err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    mAllocator.Clear();

    auto jsonImageManifest = new (&mAllocator) ImageManifest;

    memset(jsonImageManifest, 0, sizeof(ImageManifest));

    jsonImageManifest->mediaType            = "";
    jsonImageManifest->config.mediaType     = "";
    jsonImageManifest->config.digest        = "";
    jsonImageManifest->aosService.mediaType = "";
    jsonImageManifest->aosService.digest    = "";

    int ret = json_obj_parse(mJsonFileContent.Get(), mJsonFileContent.Size(), ImageManifestDescr,
        ARRAY_SIZE(ImageManifestDescr), jsonImageManifest);
    if (ret < 0) {
        return AOS_ERROR_WRAP(ret);
    }

    manifest.mSchemaVersion = jsonImageManifest->schemaVersion;
    manifest.mMediaType     = jsonImageManifest->mediaType;

    // config

    int64_t size = 0;

    if (jsonImageManifest->config.size.start) {
        auto configSize
            = aos::String(jsonImageManifest->config.size.start, jsonImageManifest->config.size.length).ToInt64();
        if (!configSize.mError.IsNone()) {
            return AOS_ERROR_WRAP(configSize.mError);
        }

        size = configSize.mValue;
    }

    manifest.mConfig = {jsonImageManifest->config.mediaType, jsonImageManifest->config.digest, size};

    // layers

    for (size_t i = 0; i < jsonImageManifest->layersLen; i++) {
        size = 0;

        if (jsonImageManifest->layers[i].size.start) {
            auto layerSize
                = aos::String(jsonImageManifest->layers[i].size.start, jsonImageManifest->layers[i].size.length)
                      .ToInt64();
            if (!layerSize.mError.IsNone()) {
                return AOS_ERROR_WRAP(layerSize.mError);
            }

            size = layerSize.mValue;
        }

        manifest.mLayers.PushBack({jsonImageManifest->layers[i].mediaType, jsonImageManifest->layers[i].digest, size});
    }

    // aosService

    if (ret & eImageManifestAosServiceField) {
        if (manifest.mAosService == nullptr) {
            return AOS_ERROR_WRAP(aos::ErrorEnum::eNoMemory);
        }

        size = 0;

        if (jsonImageManifest->aosService.size.start) {
            auto serviceSize
                = aos::String(jsonImageManifest->aosService.size.start, jsonImageManifest->aosService.size.length)
                      .ToInt64();
            if (!serviceSize.mError.IsNone()) {
                return AOS_ERROR_WRAP(serviceSize.mError);
            }

            size = serviceSize.mValue;
        }

        *manifest.mAosService = {jsonImageManifest->aosService.mediaType, jsonImageManifest->aosService.digest, size};
    }

    return aos::ErrorEnum::eNone;
}

// cppcheck-suppress unusedFunction
aos::Error OCISpec::SaveImageManifest(const aos::String& path, const aos::oci::ImageManifest& manifest)
{
    aos::LockGuard lock(mMutex);

    LOG_DBG() << "Save image manifest: " << path;

    mAllocator.Clear();

    auto jsonImageManifest = new (&mAllocator) ImageManifest;

    memset(jsonImageManifest, 0, sizeof(ImageManifest));

    jsonImageManifest->aosService.mediaType = "";
    jsonImageManifest->aosService.digest    = "";

    jsonImageManifest->schemaVersion = manifest.mSchemaVersion;
    jsonImageManifest->mediaType     = manifest.mMediaType.CStr();

    // config

    auto configSize = new (&mAllocator) aos::StaticString<20>;

    auto err = configSize->Convert(manifest.mConfig.mSize);
    if (!err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    jsonImageManifest->config = {manifest.mConfig.mMediaType.CStr(), manifest.mConfig.mDigest.CStr(),
        {const_cast<char*>(configSize->CStr()), configSize->Size()}};

    // layers

    jsonImageManifest->layersLen = manifest.mLayers.Size();

    for (size_t i = 0; i < jsonImageManifest->layersLen; i++) {
        auto layerSize = new (&mAllocator) aos::StaticString<20>;

        err = layerSize->Convert(manifest.mLayers[i].mSize);
        if (!err.IsNone()) {
            return AOS_ERROR_WRAP(err);
        }

        jsonImageManifest->layers[i] = {manifest.mLayers[i].mMediaType.CStr(), manifest.mLayers[i].mDigest.CStr(),
            {const_cast<char*>(layerSize->CStr()), layerSize->Size()}};
    }

    // aosService

    if (manifest.mAosService) {
        auto serviceSize = new (&mAllocator) aos::StaticString<20>;

        err = serviceSize->Convert(manifest.mAosService->mSize);
        if (!err.IsNone()) {
            return AOS_ERROR_WRAP(err);
        }

        jsonImageManifest->aosService = {manifest.mAosService->mMediaType.CStr(), manifest.mAosService->mDigest.CStr(),
            {const_cast<char*>(serviceSize->CStr()), serviceSize->Size()}};
    }

    auto ret = json_obj_encode_buf(ImageManifestDescr, ARRAY_SIZE(ImageManifestDescr), jsonImageManifest,
        mJsonFileContent.Get(), mJsonFileContent.MaxSize());
    if (ret < 0) {
        return AOS_ERROR_WRAP(ret);
    }

    err = mJsonFileContent.Resize(strlen(mJsonFileContent.CStr()));
    if (!err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    err = aos::FS::WriteStringToFile(path, mJsonFileContent, S_IRUSR | S_IWUSR);
    if (!err.IsNone()) {
        return err;
    }

    return aos::ErrorEnum::eNone;
}

// cppcheck-suppress unusedFunction
aos::Error OCISpec::LoadImageSpec(const aos::String& path, aos::oci::ImageSpec& imageSpec)
{
    aos::LockGuard lock(mMutex);

    LOG_DBG() << "Load image spec: " << path;

    auto err = aos::FS::ReadFileToString(path, mJsonFileContent);
    if (!err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    mAllocator.Clear();

    auto jsonImageSpec = new (&mAllocator) ImageSpec;

    memset(jsonImageSpec, 0, sizeof(ImageSpec));

    int ret = json_obj_parse(
        mJsonFileContent.Get(), mJsonFileContent.Size(), ImageSpecDescr, ARRAY_SIZE(ImageSpecDescr), jsonImageSpec);
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

// cppcheck-suppress unusedFunction
aos::Error OCISpec::SaveImageSpec(const aos::String& path, const aos::oci::ImageSpec& imageSpec)
{
    aos::LockGuard lock(mMutex);

    LOG_DBG() << "Save image spec: " << path;

    mAllocator.Clear();

    auto jsonImageSpec = new (&mAllocator) ImageSpec;

    memset(jsonImageSpec, 0, sizeof(ImageSpec));

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

    auto ret = json_obj_encode_buf(
        ImageSpecDescr, ARRAY_SIZE(ImageSpecDescr), jsonImageSpec, mJsonFileContent.Get(), mJsonFileContent.MaxSize());
    if (ret < 0) {
        return AOS_ERROR_WRAP(ret);
    }

    auto err = mJsonFileContent.Resize(strlen(mJsonFileContent.CStr()));
    if (!err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    err = aos::FS::WriteStringToFile(path, mJsonFileContent, S_IRUSR | S_IWUSR);
    if (!err.IsNone()) {
        return err;
    }

    return aos::ErrorEnum::eNone;
}

// cppcheck-suppress unusedFunction
aos::Error OCISpec::LoadRuntimeSpec(const aos::String& path, aos::oci::RuntimeSpec& runtimeSpec)
{
    aos::LockGuard lock(mMutex);

    LOG_DBG() << "Load runtime spec: " << path;

    auto err = aos::FS::ReadFileToString(path, mJsonFileContent);
    if (!err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    mAllocator.Clear();

    auto jsonRuntimeSpec = new (&mAllocator) RuntimeSpec;

    memset(jsonRuntimeSpec, 0, sizeof(RuntimeSpec));

    jsonRuntimeSpec->ociVersion             = "";
    jsonRuntimeSpec->vm.hypervisor.path     = "";
    jsonRuntimeSpec->vm.kernel.path         = "";
    jsonRuntimeSpec->vm.hwConfig.deviceTree = "";

    int ret = json_obj_parse(mJsonFileContent.Get(), mJsonFileContent.Size(), RuntimeSpecDescr,
        ARRAY_SIZE(RuntimeSpecDescr), jsonRuntimeSpec);
    if (ret < 0) {
        return AOS_ERROR_WRAP(ret);
    }

    runtimeSpec.mOCIVersion = jsonRuntimeSpec->ociVersion;

    if (ret & eRuntimeVMField) {
        if (runtimeSpec.mVM == nullptr) {
            return AOS_ERROR_WRAP(aos::ErrorEnum::eNoMemory);
        }

        runtimeSpec.mVM->mHypervisor.mPath = jsonRuntimeSpec->vm.hypervisor.path;
        runtimeSpec.mVM->mKernel.mPath     = jsonRuntimeSpec->vm.kernel.path;

        for (size_t i = 0; i < jsonRuntimeSpec->vm.hypervisor.parametersLen; i++) {
            runtimeSpec.mVM->mHypervisor.mParameters.PushBack(jsonRuntimeSpec->vm.hypervisor.parameters[i]);
        }

        for (size_t i = 0; i < jsonRuntimeSpec->vm.kernel.parametersLen; i++) {
            runtimeSpec.mVM->mKernel.mParameters.PushBack(jsonRuntimeSpec->vm.kernel.parameters[i]);
        }

        runtimeSpec.mVM->mHWConfig.mDeviceTree = jsonRuntimeSpec->vm.hwConfig.deviceTree;
        runtimeSpec.mVM->mHWConfig.mVCPUs      = jsonRuntimeSpec->vm.hwConfig.vcpus;

        uint64_t memKB = 0;

        if (jsonRuntimeSpec->vm.hwConfig.memKB.start) {
            auto result
                = aos::String(jsonRuntimeSpec->vm.hwConfig.memKB.start, jsonRuntimeSpec->vm.hwConfig.memKB.length)
                      .ToUint64();
            if (!result.mError.IsNone()) {
                return AOS_ERROR_WRAP(result.mError);
            }

            memKB = result.mValue;
        }

        runtimeSpec.mVM->mHWConfig.mMemKB = memKB;

        for (size_t i = 0; i < jsonRuntimeSpec->vm.hwConfig.dtdevsLen; i++) {
            runtimeSpec.mVM->mHWConfig.mDTDevs.PushBack(jsonRuntimeSpec->vm.hwConfig.dtdevs[i]);
        }

        for (size_t i = 0; i < jsonRuntimeSpec->vm.hwConfig.iomemsLen; i++) {
            aos::oci::VMHWConfigIOMEM ioMem {};
            auto                      jsonIOMem = jsonRuntimeSpec->vm.hwConfig.iomems[i];

            if (jsonIOMem.firstGFN.start) {
                auto result = aos::String(jsonIOMem.firstGFN.start, jsonIOMem.firstGFN.length).ToUint64();
                if (!result.mError.IsNone()) {
                    return AOS_ERROR_WRAP(result.mError);
                }

                ioMem.mFirstGFN = result.mValue;
            }

            if (jsonIOMem.firstMFN.start) {
                auto result = aos::String(jsonIOMem.firstMFN.start, jsonIOMem.firstMFN.length).ToUint64();
                if (!result.mError.IsNone()) {
                    return AOS_ERROR_WRAP(result.mError);
                }

                ioMem.mFirstMFN = result.mValue;
            }

            if (jsonIOMem.nrMFNs.start) {
                auto result = aos::String(jsonIOMem.nrMFNs.start, jsonIOMem.nrMFNs.length).ToUint64();
                if (!result.mError.IsNone()) {
                    return AOS_ERROR_WRAP(result.mError);
                }

                ioMem.mNrMFNs = result.mValue;
            }

            runtimeSpec.mVM->mHWConfig.mIOMEMs.PushBack(ioMem);
        }

        for (size_t i = 0; i < jsonRuntimeSpec->vm.hwConfig.irqsLen; i++) {
            runtimeSpec.mVM->mHWConfig.mIRQs.PushBack(jsonRuntimeSpec->vm.hwConfig.irqs[i]);
        }
    }

    return aos::ErrorEnum::eNone;
}

// cppcheck-suppress unusedFunction
aos::Error OCISpec::SaveRuntimeSpec(const aos::String& path, const aos::oci::RuntimeSpec& runtimeSpec)
{
    aos::LockGuard lock(mMutex);

    LOG_DBG() << "Save runtime spec: " << path;

    mAllocator.Clear();

    auto jsonRuntimeSpec = new (&mAllocator) RuntimeSpec;

    memset(jsonRuntimeSpec, 0, sizeof(RuntimeSpec));

    jsonRuntimeSpec->vm.hypervisor.path     = "";
    jsonRuntimeSpec->vm.kernel.path         = "";
    jsonRuntimeSpec->vm.hwConfig.deviceTree = "";
    jsonRuntimeSpec->ociVersion             = runtimeSpec.mOCIVersion.CStr();

    if (runtimeSpec.mVM) {
        jsonRuntimeSpec->vm.hypervisor.path          = const_cast<char*>(runtimeSpec.mVM->mHypervisor.mPath.CStr());
        jsonRuntimeSpec->vm.kernel.path              = const_cast<char*>(runtimeSpec.mVM->mKernel.mPath.CStr());
        jsonRuntimeSpec->vm.hypervisor.parametersLen = runtimeSpec.mVM->mHypervisor.mParameters.Size();
        jsonRuntimeSpec->vm.kernel.parametersLen     = runtimeSpec.mVM->mKernel.mParameters.Size();

        for (size_t i = 0; i < runtimeSpec.mVM->mHypervisor.mParameters.Size(); i++) {
            jsonRuntimeSpec->vm.hypervisor.parameters[i]
                = const_cast<char*>(runtimeSpec.mVM->mHypervisor.mParameters[i].CStr());
        }

        for (size_t i = 0; i < runtimeSpec.mVM->mKernel.mParameters.Size(); i++) {
            jsonRuntimeSpec->vm.kernel.parameters[i]
                = const_cast<char*>(runtimeSpec.mVM->mKernel.mParameters[i].CStr());
        }

        jsonRuntimeSpec->vm.hwConfig.deviceTree = runtimeSpec.mVM->mHWConfig.mDeviceTree.CStr();
        jsonRuntimeSpec->vm.hwConfig.vcpus      = runtimeSpec.mVM->mHWConfig.mVCPUs;

        auto memKB = new (&mAllocator) aos::StaticString<20>;

        auto err = memKB->Convert(runtimeSpec.mVM->mHWConfig.mMemKB);
        if (!err.IsNone()) {
            return AOS_ERROR_WRAP(err);
        }

        jsonRuntimeSpec->vm.hwConfig.memKB = {const_cast<char*>(memKB->CStr()), memKB->Size()};

        jsonRuntimeSpec->vm.hwConfig.dtdevsLen = runtimeSpec.mVM->mHWConfig.mDTDevs.Size();

        for (size_t i = 0; i < jsonRuntimeSpec->vm.hwConfig.dtdevsLen; i++) {
            jsonRuntimeSpec->vm.hwConfig.dtdevs[i] = runtimeSpec.mVM->mHWConfig.mDTDevs[i].CStr();
        }

        jsonRuntimeSpec->vm.hwConfig.iomemsLen = runtimeSpec.mVM->mHWConfig.mIOMEMs.Size();

        for (size_t i = 0; i < jsonRuntimeSpec->vm.hwConfig.iomemsLen; i++) {
            auto iomem = runtimeSpec.mVM->mHWConfig.mIOMEMs[i];

            auto firstGFN = new (&mAllocator) aos::StaticString<20>;

            err = firstGFN->Convert(iomem.mFirstGFN);
            if (!err.IsNone()) {
                return AOS_ERROR_WRAP(err);
            }

            auto firstMFN = new (&mAllocator) aos::StaticString<20>;

            err = firstMFN->Convert(iomem.mFirstMFN);
            if (!err.IsNone()) {
                return AOS_ERROR_WRAP(err);
            }

            auto nrMFNs = new (&mAllocator) aos::StaticString<20>;

            err = nrMFNs->Convert(iomem.mNrMFNs);
            if (!err.IsNone()) {
                return AOS_ERROR_WRAP(err);
            }

            jsonRuntimeSpec->vm.hwConfig.iomems[i] = {
                {const_cast<char*>(firstGFN->CStr()), firstGFN->Size()},
                {const_cast<char*>(firstMFN->CStr()), firstMFN->Size()},
                {const_cast<char*>(nrMFNs->CStr()), nrMFNs->Size()},
            };
        }

        jsonRuntimeSpec->vm.hwConfig.irqsLen = runtimeSpec.mVM->mHWConfig.mIRQs.Size();

        for (size_t i = 0; i < jsonRuntimeSpec->vm.hwConfig.irqsLen; i++) {
            jsonRuntimeSpec->vm.hwConfig.irqs[i] = runtimeSpec.mVM->mHWConfig.mIRQs[i];
        }
    }

    auto ret = json_obj_encode_buf(RuntimeSpecDescr, ARRAY_SIZE(RuntimeSpecDescr), jsonRuntimeSpec,
        mJsonFileContent.Get(), mJsonFileContent.MaxSize());
    if (ret < 0) {
        return AOS_ERROR_WRAP(ret);
    }

    auto err = mJsonFileContent.Resize(strlen(mJsonFileContent.CStr()));
    if (!err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    err = aos::FS::WriteStringToFile(path, mJsonFileContent, S_IRUSR | S_IWUSR);
    if (!err.IsNone()) {
        return err;
    }

    return aos::ErrorEnum::eNone;
}
