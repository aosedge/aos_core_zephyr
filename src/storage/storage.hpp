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

/**
 * Storage instance.
 */
class Storage : public aos::sm::launcher::StorageItf,
                public aos::sm::servicemanager::StorageItf,
                public aos::iam::certhandler::StorageItf,
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

    /**
     * Adds new certificate info to the storage.
     *
     * @param certType certificate type.
     * @param certInfo certificate information.
     * @return Error.
     */
    aos::Error AddCertInfo(const aos::String& certType, const aos::iam::certhandler::CertInfo& certInfo) override;

    /**
     * Returns information about certificate with specified issuer and serial number.
     *
     * @param issuer certificate issuer.
     * @param serial serial number.
     * @param cert result certificate.
     * @return Error.
     */
    aos::Error GetCertInfo(const aos::Array<uint8_t>& issuer, const aos::Array<uint8_t>& serial,
        aos::iam::certhandler::CertInfo& cert) override;

    /**
     * Returns info for all certificates with specified certificate type.
     *
     * @param certType certificate type.
     * @param[out] certsInfo result certificates info.
     * @return Error.
     */
    aos::Error GetCertsInfo(
        const aos::String& certType, aos::Array<aos::iam::certhandler::CertInfo>& certsInfo) override;

    /**
     * Removes certificate with specified certificate type and url.
     *
     * @param certType certificate type.
     * @param certURL certificate URL.
     * @return Error.
     */
    aos::Error RemoveCertInfo(const aos::String& certType, const aos::String& certURL) override;

    /**
     * Removes all certificates with specified certificate type.
     *
     * @param certType certificate type.
     * @return Error.
     */
    aos::Error RemoveAllCertsInfo(const aos::String& certType) override;

private:
    constexpr static auto cStoragePath = CONFIG_AOS_STORAGE_DIR;

    struct InstanceIdent {
        char     mServiceID[aos::cServiceIDLen + 1];
        char     mSubjectID[aos::cSubjectIDLen + 1];
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
        InstanceIdent mInstanceIdent;
        uint32_t      mUID;
        uint64_t      mPriority;
        char          mStoragePath[aos::cFilePathLen + 1];
        char          mStatePath[aos::cFilePathLen + 1];

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

    struct VersionInfo {
        uint64_t mAosVersion;
        char     mVendorVersion[aos::cVendorVersionLen + 1];
        char     mDescription[aos::cDescriptionLen + 1];

        /**
         * Compares version info.
         *
         * @param version info to compare.
         * @return bool.
         */
        bool operator==(const VersionInfo& rhs) const
        {
            return mAosVersion == rhs.mAosVersion && strcmp(mVendorVersion, rhs.mVendorVersion) == 0
                && strcmp(mDescription, rhs.mDescription) == 0;
        }
    };

    struct ServiceData {
        VersionInfo mVersionInfo;
        char        mServiceID[aos::cServiceIDLen + 1];
        char        mProviderID[aos::cProviderIDLen + 1];
        char        mImagePath[aos::cFilePathLen + 1];

        /**
         * Compares service data.
         *
         * @param service data to compare.
         * @return bool.
         */
        bool operator==(const ServiceData& rhs) const
        {
            return mVersionInfo == rhs.mVersionInfo && strcmp(mServiceID, rhs.mServiceID) == 0
                && strcmp(mProviderID, rhs.mProviderID) == 0 && strcmp(mImagePath, rhs.mImagePath) == 0;
        }
    };

    struct CertInfo {
        uint8_t  mIssuer[aos::crypto::cCertIssuerSize];
        size_t   mIssuerSize;
        uint8_t  mSerial[aos::crypto::cSerialNumSize];
        size_t   mSerialSize;
        char     mCertURL[aos::cURLLen + 1];
        char     mKeyURL[aos::cURLLen + 1];
        char     mCertType[aos::iam::certhandler::cCertTypeLen + 1];
        uint64_t mNotAfter;

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
                && strcmp(mCertType, rhs.mCertType) == 0 && mNotAfter == rhs.mNotAfter;
        }
    };

    InstanceInfo                         ConvertInstanceInfo(const aos::InstanceInfo& instance);
    aos::InstanceInfo                    ConvertInstanceInfo(const InstanceInfo& instance);
    ServiceData                          ConvertServiceData(const aos::sm::servicemanager::ServiceData& service);
    aos::sm::servicemanager::ServiceData ConvertServiceData(const ServiceData& service);
    CertInfo ConvertCertInfo(const aos::String& certType, const aos::iam::certhandler::CertInfo& certInfo);
    aos::iam::certhandler::CertInfo ConvertCertInfo(const CertInfo& certInfo);

    FileStorage<InstanceInfo> mInstanceDatabase;
    FileStorage<ServiceData>  mServiceDatabase;
    FileStorage<CertInfo>     mCertDatabase;
    aos::Mutex                mMutex;
};

#endif
