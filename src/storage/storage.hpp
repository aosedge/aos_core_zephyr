/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef STORAGE_HPP_
#define STORAGE_HPP_

#include <aos/common/tools/thread.hpp>
#include <aos/sm/launcher.hpp>
#include <aos/sm/servicemanager.hpp>

#include "filestorage.hpp"

/**
 * Storage instance.
 */
class Storage : public aos::sm::launcher::StorageItf,
                public aos::sm::servicemanager::StorageItf,
                private aos::NonCopyable {
public:
    /**
     * Initializes storage instance.
     * @return aos::Error.
     */
    aos::Error Init();

    /**
     * Adds new instance to storage.
     *
     * @param instance instance to add.
     * @return Error.
     */
    aos::Error AddInstance(const aos::InstanceInfo& instance) override;

    /**
     * Updates previously stored instance.
     *
     * @param instance instance to update.
     * @return Error.
     */
    aos::Error UpdateInstance(const aos::InstanceInfo& instance) override;

    /**
     * Removes previously stored instance.
     *
     * @param instanceID instance ID to remove.
     * @return Error.
     */
    aos::Error RemoveInstance(const aos::InstanceIdent& instanceIdent) override;

    /**
     * Returns all stored instances.
     *
     * @param instances array to return stored instances.
     * @return Error.
     */
    aos::Error GetAllInstances(aos::Array<aos::InstanceInfo>& instances) override;

    /**
     * Adds new service to storage.
     *
     * @param service service to add.
     * @return Error.
     */
    aos::Error AddService(const aos::sm::servicemanager::ServiceData& service) override;

    /**
     * Updates previously stored service.
     *
     * @param service service to update.
     * @return Error.
     */
    aos::Error UpdateService(const aos::sm::servicemanager::ServiceData& service) override;

    /**
     * Removes previously stored service.
     *
     * @param serviceID service ID to remove.
     * @param aosVersion Aos service version.
     * @return Error.
     */
    aos::Error RemoveService(const aos::String& serviceID, uint64_t aosVersion) override;

    /**
     * Returns all stored services.
     *
     * @param services array to return stored services.
     * @return Error.
     */
    aos::Error GetAllServices(aos::Array<aos::sm::servicemanager::ServiceData>& services) override;

    /**
     * Returns service data by service ID.
     *
     * @param serviceID service ID.
     * @return  RetWithError<ServiceData>.
     */
    aos::RetWithError<aos::sm::servicemanager::ServiceData> GetService(const aos::String& serviceID) override;

private:
    constexpr static const char* cStoragePath = CONFIG_AOS_STORAGE_PATH;

    struct InstanceIdent {
        char     mServiceID[aos::cServiceIDLen];
        char     mSubjectID[aos::cSubjectIDLen];
        uint64_t mInstance;
    };

    struct InstanceInfo {
        InstanceIdent mInstanceIdent;
        uint32_t      mUID;
        uint64_t      mPriority;
        char          mStoragePath[aos::cFilePathLen];
        char          mStatePath[aos::cFilePathLen];
    };

    struct VersionInfo {
        uint64_t mAosVersion;
        char     mVendorVersion[aos::cVendorVersionLen];
        char     mDescription[aos::cDescriptionLen];
    };

    struct ServiceData {
        VersionInfo mVersionInfo;
        char        mServiceID[aos::cServiceIDLen];
        char        mProviderID[aos::cProviderIDLen];
        char        mImagePath[aos::cFilePathLen];
    };

    InstanceInfo                         ConvertInstanceInfo(const aos::InstanceInfo& instance);
    aos::InstanceInfo                    ConvertInstanceInfo(const InstanceInfo& instance);
    ServiceData                          ConvertServiceData(const aos::sm::servicemanager::ServiceData& service);
    aos::sm::servicemanager::ServiceData ConvertServiceData(const ServiceData& service);

    FileStorage<InstanceInfo> mInstanceDatabase;
    FileStorage<ServiceData>  mServiceDatabase;
    aos::Mutex                mMutex;
};

#endif
