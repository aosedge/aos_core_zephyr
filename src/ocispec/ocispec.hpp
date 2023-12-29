/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef OCISPEC_HPP_
#define OCISPEC_HPP_

#include <zephyr/data/json.h>

#include <aos/common/ocispec.hpp>
#include <aos/common/tools/allocator.hpp>
#include <aos/common/tools/thread.hpp>

// Image spec

/**
 * OCI content descriptor.
 */

struct ContentDescriptor {
    const char*    mediaType;
    const char*    digest;
    json_obj_token size;
};

/**
 * OCI image manifest.
 */
struct ImageManifest {
    int               schemaVersion;
    const char*       mediaType;
    ContentDescriptor config;
    ContentDescriptor layers[aos::cMaxNumLayers];
    size_t            layersLen;
    ContentDescriptor aosService;
};

/**
 * OCI image manifest bit fields.
 */
enum ImageManifestFields {
    eImageManifestSchemaVersionField = BIT(0),
    eImageManifestMediaTypeField     = BIT(1),
    eImageManifestConfigField        = BIT(2),
    eImageManifestLayersField        = BIT(3),
    eImageManifestAosServiceField    = BIT(4),
};

/**
 * OCI image config.
 */
struct ImageConfig {
    const char* Cmd[aos::oci::cMaxParamCount];
    size_t      cmdLen;
    const char* Env[aos::oci::cMaxParamCount];
    size_t      envLen;
    const char* Entrypoint[aos::oci::cMaxParamCount];
    size_t      entrypointLen;
};

/**
 * OCI image specification.
 */
struct ImageSpec {
    ImageConfig config;
};

// Runtime spec

/**
 * Contains information about the hypervisor to use for a virtual machine.
 */
struct VMHypervisor {
    const char* path;
    const char* parameters[aos::oci::cMaxParamCount];
    size_t      parametersLen;
};

/**
 * Contains information about the kernel to use for a virtual machine.
 */
struct VMKernel {
    const char* path;
    const char* parameters[aos::oci::cMaxParamCount];
    size_t      parametersLen;
};

/**
 * Contains information about IOMEMs.
 */
struct VMHWConfigIOMEM {
    json_obj_token firstGFN;
    json_obj_token firstMFN;
    json_obj_token nrMFNs;
};

/**
 * Contains information about HW configuration.
 */
struct VMHWConfig {
    const char*     deviceTree;
    uint32_t        vcpus;
    json_obj_token  memKB;
    const char*     dtdevs[aos::oci::cMaxDTDevsCount];
    size_t          dtdevsLen;
    VMHWConfigIOMEM iomems[aos::oci::cMaxIOMEMsCount];
    size_t          iomemsLen;
    uint32_t        irqs[aos::oci::cMaxIRQsCount];
    size_t          irqsLen;
};

/**
 * Contains information for virtual-machine-based containers.
 */
struct VM {
    VMHypervisor hypervisor;
    VMKernel     kernel;
    VMHWConfig   hwConfig;
};

/**
 * OCI runtime specification bit fields.
 */
enum RuntimeSpecFields {
    eRuntimeOCIVersionField = BIT(0),
    eRuntimeVMField         = BIT(1),
};

/**
 * OCI runtime specification.
 */
struct RuntimeSpec {
    const char* ociVersion;
    VM          vm;
};

/**
 * OCISpec instance.
 */
class OCISpec : public aos::OCISpecItf {
public:
    /**
     * Loads OCI image manifest.
     *
     * @param path file path.
     * @param manifest image manifest.
     * @return Error.
     */
    aos::Error LoadImageManifest(const aos::String& path, aos::oci::ImageManifest& manifest) override;

    /**
     * Saves OCI image manifest.
     *
     * @param path file path.
     * @param manifest image manifest.
     * @return Error.
     */
    aos::Error SaveImageManifest(const aos::String& path, const aos::oci::ImageManifest& manifest) override;

    /**
     * Loads OCI image spec.
     *
     * @param path file path.
     * @param imageSpec image spec.
     * @return Error.
     */
    aos::Error LoadImageSpec(const aos::String& path, aos::oci::ImageSpec& imageSpec) override;

    /**
     * Saves OCI image spec.
     *
     * @param path file path.
     * @param imageSpec image spec.
     * @return Error.
     */
    aos::Error SaveImageSpec(const aos::String& path, const aos::oci::ImageSpec& imageSpec) override;

    /**
     * Loads OCI runtime spec.
     *
     * @param path file path.
     * @param runtimeSpec runtime spec.
     * @return Error.
     */
    aos::Error LoadRuntimeSpec(const aos::String& path, aos::oci::RuntimeSpec& runtimeSpec) override;

    /**
     * Saves OCI runtime spec.
     *
     * @param path file path.
     * @param runtimeSpec runtime spec.
     * @return Error.
     */
    virtual aos::Error SaveRuntimeSpec(const aos::String& path, const aos::oci::RuntimeSpec& runtimeSpec) override;

private:
    static constexpr size_t cJsonMaxContentSize = 4096;
    static constexpr size_t cAllocationSize     = 2048;
    static constexpr size_t cMaxNumAllocations  = 32;

    aos::RetWithError<size_t> ReadFileContentToBuffer(const aos::String& path);
    aos::Error                WriteEncodedJsonBufferToFile(const aos::String& path);

    aos::Mutex                                                mMutex;
    aos::StaticBuffer<cJsonMaxContentSize>                    mJsonFileBuffer;
    aos::StaticAllocator<cAllocationSize, cMaxNumAllocations> mAllocator;
};

#endif
