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

namespace aos::zephyr::ocispec {

// Image spec

/**
 * OCI content descriptor.
 */

struct ContentDescriptor {
    const char* mediaType;
    const char* digest;
    uint64_t    size;
};

/**
 * OCI image manifest.
 */
struct ImageManifest {
    int               schemaVersion;
    const char*       mediaType;
    ContentDescriptor config;
    ContentDescriptor layers[cMaxNumLayers];
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
    const char* Cmd[oci::cMaxParamCount];
    size_t      cmdLen;
    const char* Env[oci::cMaxParamCount];
    size_t      envLen;
    const char* Entrypoint[oci::cMaxParamCount];
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
    const char* parameters[oci::cMaxParamCount];
    size_t      parametersLen;
};

/**
 * Contains information about the kernel to use for a virtual machine.
 */
struct VMKernel {
    const char* path;
    const char* parameters[oci::cMaxParamCount];
    size_t      parametersLen;
};

/**
 * Contains information about IOMEMs.
 */
struct VMHWConfigIOMEM {
    uint64_t firstGFN;
    uint64_t firstMFN;
    uint64_t nrMFNs;
};

/**
 * Contains information about HW configuration.
 */
struct VMHWConfig {
    const char*     deviceTree;
    uint32_t        vcpus;
    uint64_t        memKB;
    const char*     dtdevs[oci::cMaxDTDevsCount];
    size_t          dtdevsLen;
    VMHWConfigIOMEM iomems[oci::cMaxIOMEMsCount];
    size_t          iomemsLen;
    uint32_t        irqs[oci::cMaxIRQsCount];
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
class OCISpec : public OCISpecItf {
public:
    /**
     * Loads OCI image manifest.
     *
     * @param path file path.
     * @param manifest image manifest.
     * @return Error.
     */
    Error LoadImageManifest(const String& path, oci::ImageManifest& manifest) override;

    /**
     * Saves OCI image manifest.
     *
     * @param path file path.
     * @param manifest image manifest.
     * @return Error.
     */
    Error SaveImageManifest(const String& path, const oci::ImageManifest& manifest) override;

    /**
     * Loads OCI image spec.
     *
     * @param path file path.
     * @param imageSpec image spec.
     * @return Error.
     */
    Error LoadImageSpec(const String& path, oci::ImageSpec& imageSpec) override;

    /**
     * Saves OCI image spec.
     *
     * @param path file path.
     * @param imageSpec image spec.
     * @return Error.
     */
    Error SaveImageSpec(const String& path, const oci::ImageSpec& imageSpec) override;

    /**
     * Loads OCI runtime spec.
     *
     * @param path file path.
     * @param runtimeSpec runtime spec.
     * @return Error.
     */
    Error LoadRuntimeSpec(const String& path, oci::RuntimeSpec& runtimeSpec) override;

    /**
     * Saves OCI runtime spec.
     *
     * @param path file path.
     * @param runtimeSpec runtime spec.
     * @return Error.
     */
    virtual Error SaveRuntimeSpec(const String& path, const oci::RuntimeSpec& runtimeSpec) override;

private:
    static constexpr size_t cJsonMaxContentLen = 4096;
    static constexpr size_t cAllocationSize    = 2048;
    static constexpr size_t cMaxNumAllocations = 32;

    Mutex                                                mMutex;
    StaticString<cJsonMaxContentLen>                     mJsonFileContent;
    StaticAllocator<cAllocationSize, cMaxNumAllocations> mAllocator;
};

} // namespace aos::zephyr::ocispec

#endif
