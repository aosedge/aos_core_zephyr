/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef OCISPEC_HPP_
#define OCISPEC_HPP_

#include <aos/common/tools/thread.hpp>

#include "ocijsonspec.hpp"

static constexpr size_t cJsonMaxContentSize = 4096;

/**
 * OCISpec instance.
 */
class OCISpec : public aos::OCISpecItf {
public:
    OCISpec()
        : mJsonFileBuffer()
        , mBufferMutex()
        , mRuntimeSpec()
        , mImageSpec()
    {
    }
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
    aos::RetWithError<int> ReadFileContentToBuffer(const aos::String& path, size_t maxContentSize);
    aos::Error             WriteEncodedJsonBufferToFile(const aos::String& path, size_t len);

    aos::StaticBuffer<cJsonMaxContentSize> mJsonFileBuffer;
    aos::Mutex                             mBufferMutex;
    RuntimeSpec                            mRuntimeSpec;
    ImageSpec                              mImageSpec;
};
#endif
