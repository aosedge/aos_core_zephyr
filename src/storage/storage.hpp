/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef STORAGE_HPP_
#define STORAGE_HPP_

#include <aos/common/tools/thread.hpp>
#include <aos/iam/certmodules/certmodule.hpp>
#include <aos/sm/launcher.hpp>
#include <aos/sm/servicemanager.hpp>

#include "filestorage.hpp"

namespace aos::zephyr::storage {

/**
 * Storage instance.
 */
class Storage : public sm::launcher::StorageItf,
                public sm::servicemanager::StorageItf,
                public iam::certhandler::StorageItf,
                private NonCopyable {
public:
    /**
     * Initializes storage instance.
     * @return Error.
     */
    Error Init();

    /**
     * Adds new instance to storage.
     *
     * @param instance instance to add.
     * @return Error.
     */
    Error AddInstance(const aos::InstanceInfo& instance) override;

    /**
     * Updates previously stored instance.
     *
     * @param instance instance to update.
     * @return Error.
     */
    Error UpdateInstance(const aos::InstanceInfo& instance) override;

    /**
     * Removes previously stored instance.
     *
     * @param instanceID instance ID to remove.
     * @return Error.
     */
    Error RemoveInstance(const aos::InstanceIdent& instanceIdent) override;

    /**
     * Returns all stored instances.
     *
     * @param instances array to return stored instances.
     * @return Error.
     */
    Error GetAllInstances(Array<aos::InstanceInfo>& instances) override;

    /**
     * Adds new service to storage.
     *
     * @param service service to add.
     * @return Error.
     */
    Error AddService(const sm::servicemanager::ServiceData& service) override;

    /**
     * Updates previously stored service.
     *
     * @param service service to update.
     * @return Error.
     */
    Error UpdateService(const sm::servicemanager::ServiceData& service) override;

    /**
     * Removes previously stored service.
     *
     * @param serviceID service ID to remove.
     * @param aosVersion Aos service version.
     * @return Error.
     */
    Error RemoveService(const String& serviceID, const String& version) override;

    /**
     * Returns all stored services.
     *
     * @param services array to return stored services.
     * @return Error.
     */
    Error GetAllServices(Array<sm::servicemanager::ServiceData>& services) override;

    /**
     * Returns service data by service ID.
     *
     * @param serviceID service ID.
     * @return  RetWithError<ServiceData>.
     */
    RetWithError<sm::servicemanager::ServiceData> GetService(const String& serviceID) override;

    /**
     * Adds new certificate info to the storage.
     *
     * @param certType certificate type.
     * @param certInfo certificate information.
     * @return Error.
     */
    Error AddCertInfo(const String& certType, const iam::certhandler::CertInfo& certInfo) override;

    /**
     * Returns information about certificate with specified issuer and serial number.
     *
     * @param issuer certificate issuer.
     * @param serial serial number.
     * @param cert result certificate.
     * @return Error.
     */
    Error GetCertInfo(
        const Array<uint8_t>& issuer, const Array<uint8_t>& serial, iam::certhandler::CertInfo& cert) override;

    /**
     * Returns info for all certificates with specified certificate type.
     *
     * @param certType certificate type.
     * @param[out] certsInfo result certificates info.
     * @return Error.
     */
    Error GetCertsInfo(const String& certType, Array<iam::certhandler::CertInfo>& certsInfo) override;

    /**
     * Removes certificate with specified certificate type and url.
     *
     * @param certType certificate type.
     * @param certURL certificate URL.
     * @return Error.
     */
    Error RemoveCertInfo(const String& certType, const String& certURL) override;

    /**
     * Removes all certificates with specified certificate type.
     *
     * @param certType certificate type.
     * @return Error.
     */
    Error RemoveAllCertsInfo(const String& certType) override;

private:
    constexpr static auto cStoragePath = CONFIG_AOS_STORAGE_DIR;

    struct InstanceIdent {
        char     mServiceID[cServiceIDLen + 1];
        char     mSubjectID[cSubjectIDLen + 1];
        uint64_t mInstance;

        /**
         * Compares instance ident.
         *
         * @param instance ident to compare.
         * @return bool.
         */
        bool operator==(const InstanceIdent& instance) const
        {
            return strcmp(mServiceID, instance.mServiceID) == 0 && strcmp(mSubjectID, instance.mSubjectID) == 0
                && mInstance == instance.mInstance;
        }
    };

    struct InstanceInfo {
        Storage::InstanceIdent mInstanceIdent;
        uint32_t               mUID;
        uint64_t               mPriority;
        char                   mStoragePath[cFilePathLen + 1];
        char                   mStatePath[cFilePathLen + 1];

        /**
         * Compares instance info.
         *
         * @param instance info to compare.
         * @return bool.
         */
        bool operator==(const InstanceInfo& rhs) const
        {
            return mInstanceIdent == rhs.mInstanceIdent && mPriority == rhs.mPriority && mUID == rhs.mUID
                && strcmp(mStoragePath, rhs.mStoragePath) == 0 && strcmp(mStatePath, rhs.mStatePath) == 0;
        }
    };

    struct ServiceData {
        char mServiceID[cServiceIDLen + 1];
        char mProviderID[cProviderIDLen + 1];
        char mVersion[cVersionLen + 1];
        char mImagePath[cFilePathLen + 1];

        /**
         * Compares service data.
         *
         * @param service data to compare.
         * @return bool.
         */
        bool operator==(const ServiceData& rhs) const
        {
            return strcmp(mServiceID, rhs.mServiceID) == 0 && strcmp(mProviderID, rhs.mProviderID) == 0
                && strcmp(mVersion, rhs.mVersion) == 0 && strcmp(mImagePath, rhs.mImagePath) == 0;
        }
    };

    struct CertInfo {
        uint8_t  mIssuer[crypto::cCertIssuerSize];
        size_t   mIssuerSize;
        uint8_t  mSerial[crypto::cSerialNumSize];
        size_t   mSerialSize;
        char     mCertURL[cURLLen + 1];
        char     mKeyURL[cURLLen + 1];
        char     mCertType[iam::certhandler::cCertTypeLen + 1];
        timespec mNotAfter;

        /**
         * Compares cert info.
         *
         * @param cert info to compare.
         * @return bool.
         */
        bool operator==(const CertInfo& rhs) const
        {
            return mIssuerSize == rhs.mIssuerSize && mSerialSize == rhs.mSerialSize
                && memcmp(mIssuer, rhs.mIssuer, mIssuerSize) == 0 && memcmp(mSerial, rhs.mSerial, mSerialSize) == 0
                && strcmp(mCertURL, rhs.mCertURL) == 0 && strcmp(mKeyURL, rhs.mKeyURL) == 0
                && strcmp(mCertType, rhs.mCertType) == 0 && mNotAfter.tv_sec == rhs.mNotAfter.tv_sec
                && mNotAfter.tv_nsec == rhs.mNotAfter.tv_nsec;
        }
    };

    UniquePtr<Storage::InstanceInfo>           ConvertInstanceInfo(const aos::InstanceInfo& instance);
    UniquePtr<aos::InstanceInfo>               ConvertInstanceInfo(const Storage::InstanceInfo& instance);
    UniquePtr<Storage::ServiceData>            ConvertServiceData(const sm::servicemanager::ServiceData& service);
    UniquePtr<sm::servicemanager::ServiceData> ConvertServiceData(const Storage::ServiceData& service);
    UniquePtr<Storage::CertInfo> ConvertCertInfo(const String& certType, const iam::certhandler::CertInfo& certInfo);
    UniquePtr<iam::certhandler::CertInfo> ConvertCertInfo(const Storage::CertInfo& certInfo);

    FileStorage<Storage::InstanceInfo> mInstanceDatabase;
    FileStorage<Storage::ServiceData>  mServiceDatabase;
    FileStorage<Storage::CertInfo>     mCertDatabase;
    Mutex                              mMutex;

    StaticAllocator<Max(sizeof(Storage::InstanceInfo), sizeof(aos::InstanceInfo))
        + Max(sizeof(Storage::ServiceData), sizeof(sm::servicemanager::ServiceData))
        + Max(sizeof(Storage::CertInfo), sizeof(iam::certhandler::CertInfo))>
        mAllocator;
};

} // namespace aos::zephyr::storage

#endif
