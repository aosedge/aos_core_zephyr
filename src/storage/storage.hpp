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
#include <aos/sm/layermanager.hpp>
#include <aos/sm/servicemanager.hpp>

#include "filestorage.hpp"

namespace aos::zephyr::storage {

/**
 * Storage instance.
 */
class Storage : public sm::launcher::StorageItf,
                public sm::servicemanager::StorageItf,
                public sm::layermanager::StorageItf,
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
    Error AddInstance(const sm::launcher::InstanceData& instance) override;

    /**
     * Updates previously stored instance.
     *
     * @param instance instance to update.
     * @return Error.
     */
    Error UpdateInstance(const sm::launcher::InstanceData& instance) override;

    /**
     * Removes previously stored instance.
     *
     * @param instanceID instance ID to remove.
     * @return Error.
     */
    Error RemoveInstance(const String& instanceID) override;

    /**
     * Returns all stored instances.
     *
     * @param instances array to return stored instances.
     * @return Error.
     */
    Error GetAllInstances(Array<sm::launcher::InstanceData>& instances) override;

    /**
     * Returns operation version.
     *
     * @return RetWithError<uint64_t>.
     */
    RetWithError<uint64_t> GetOperationVersion() const override;

    /**
     * Sets operation version.
     *
     * @param version operation version.
     * @return Error.
     */
    Error SetOperationVersion(uint64_t version) override;

    /**
     * Returns instances's override environment variables array.
     *
     * @param envVarsInstanceInfos[out] instances's override environment variables array.
     * @return Error.
     */
    Error GetOverrideEnvVars(Array<cloudprotocol::EnvVarsInstanceInfo>& envVarsInstanceInfos) const override;

    /**
     * Sets instances's override environment variables array.
     *
     * @param envVarsInstanceInfos instances's override environment variables array.
     * @return Error.
     */
    Error SetOverrideEnvVars(const Array<cloudprotocol::EnvVarsInstanceInfo>& envVarsInstanceInfos) override;

    /**
     * Returns online time.
     *
     * @return RetWithError<Time>.
     */
    RetWithError<Time> GetOnlineTime() const override;

    /**
     * Sets online time.
     *
     * @param time online time.
     * @return Error.
     */
    Error SetOnlineTime(const Time& time) override;

    /**
     * Adds new service to storage.
     *
     * @param service service to add.
     * @return Error.
     */
    Error AddService(const sm::servicemanager::ServiceData& service) override;

    /**
     * Returns service versions by service ID.
     *
     * @param serviceID service ID.
     * @param services[out] service version for the given id.
     * @return Error.
     */
    Error GetServiceVersions(const String& serviceID, Array<sm::servicemanager::ServiceData>& services) override;

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
     * @param version Aos service version.
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
     * Adds layer to storage.
     *
     * @param layer layer data to add.
     * @return Error.
     */
    Error AddLayer(const sm::layermanager::LayerData& layer) override;

    /**
     * Removes layer from storage.
     *
     * @param digest layer digest.
     * @return Error.
     */
    Error RemoveLayer(const String& digest) override;

    /**
     * Returns all stored layers.
     *
     * @param layers[out] array to return stored layers.
     * @return Error.
     */
    Error GetAllLayers(Array<sm::layermanager::LayerData>& layers) const override;

    /**
     * Returns layer data.
     *
     * @param digest layer digest.
     * @param[out] layer layer data.
     * @return Error.
     */
    Error GetLayer(const String& digest, sm::layermanager::LayerData& layer) const override;

    /**
     * Updates layer.
     *
     * @param layer layer data to update.
     * @return Error.
     */
    Error UpdateLayer(const sm::layermanager::LayerData& layer) override;

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

    struct InstanceData {
        char                   mInstanceID[cInstanceIDLen + 1];
        Storage::InstanceIdent mInstanceIdent;
        uint32_t               mUID;
        uint64_t               mPriority;
        char                   mStoragePath[cFilePathLen + 1];
        char                   mStatePath[cFilePathLen + 1];

        /**
         * Compares instance data.
         *
         * @param instance instance to compare.
         * @return bool.
         */
        bool operator==(const InstanceData& rhs) const
        {
            return strcmp(mInstanceID, rhs.mInstanceID) == 0 && mInstanceIdent == rhs.mInstanceIdent
                && mPriority == rhs.mPriority && mUID == rhs.mUID && strcmp(mStoragePath, rhs.mStoragePath) == 0
                && strcmp(mStatePath, rhs.mStatePath) == 0;
        }
    };

    struct ServiceData {
        char     mServiceID[cServiceIDLen + 1];
        char     mProviderID[cProviderIDLen + 1];
        char     mVersion[cVersionLen + 1];
        char     mImagePath[cFilePathLen + 1];
        char     mManifestDigest[oci::cMaxDigestLen + 1];
        timespec mTimestamp;
        char     mState[32];
        uint64_t mSize;
        uint32_t mGID;

        /**
         * Compares service data.
         *
         * @param service data to compare.
         * @return bool.
         */
        bool operator==(const ServiceData& rhs) const
        {
            return strcmp(mServiceID, rhs.mServiceID) == 0 && strcmp(mProviderID, rhs.mProviderID) == 0
                && strcmp(mVersion, rhs.mVersion) == 0 && strcmp(mImagePath, rhs.mImagePath) == 0
                && strcmp(mManifestDigest, rhs.mManifestDigest) == 0 && mTimestamp.tv_sec == rhs.mTimestamp.tv_sec
                && mTimestamp.tv_nsec == rhs.mTimestamp.tv_nsec && strcmp(mState, rhs.mState) == 0 && mSize == rhs.mSize
                && mGID == rhs.mGID;
        }
    };

    struct LayerData {
        char     mLayerDigest[cLayerDigestLen + 1];
        char     mUnpackedLayerDigest[cLayerDigestLen + 1];
        char     mLayerID[cLayerIDLen + 1];
        char     mVersion[cVersionLen + 1];
        char     mPath[cFilePathLen + 1];
        char     mOSVersion[cVersionLen + 1];
        timespec mTimestamp;
        char     mState[32];
        size_t   mSize;

        /**
         * Compares layer data.
         *
         * @param layer layer to compare.
         * @return bool.
         */
        bool operator==(const LayerData& rhs) const
        {
            return strcmp(mLayerDigest, rhs.mLayerDigest) == 0
                && strcmp(mUnpackedLayerDigest, rhs.mUnpackedLayerDigest) == 0 && strcmp(mLayerID, rhs.mLayerID) == 0
                && strcmp(mVersion, rhs.mVersion) == 0 && strcmp(mPath, rhs.mPath) == 0
                && strcmp(mOSVersion, rhs.mOSVersion) == 0 && mTimestamp.tv_sec == rhs.mTimestamp.tv_sec
                && mTimestamp.tv_nsec == rhs.mTimestamp.tv_nsec && strcmp(mState, rhs.mState) == 0
                && mSize == rhs.mSize;
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

    UniquePtr<Storage::InstanceData> ConvertInstanceData(const sm::launcher::InstanceData& instance);
    Error ConvertInstanceData(const Storage::InstanceData& dbInstance, sm::launcher::InstanceData& outInstance);
    UniquePtr<Storage::ServiceData> ConvertServiceData(const sm::servicemanager::ServiceData& service);
    Error ConvertServiceData(const Storage::ServiceData& storageService, sm::servicemanager::ServiceData& outService);
    UniquePtr<Storage::LayerData> ConvertLayerData(const sm::layermanager::LayerData& layer);
    Error ConvertLayerData(const Storage::LayerData& dbLayer, sm::layermanager::LayerData& outLayer) const;
    UniquePtr<Storage::CertInfo> ConvertCertInfo(const String& certType, const iam::certhandler::CertInfo& certInfo);
    UniquePtr<iam::certhandler::CertInfo> ConvertCertInfo(const Storage::CertInfo& certInfo);

    FileStorage<Storage::InstanceData>      mInstanceDatabase;
    FileStorage<Storage::ServiceData>       mServiceDatabase;
    mutable FileStorage<Storage::LayerData> mLayerDatabase;
    FileStorage<Storage::CertInfo>          mCertDatabase;
    mutable Mutex                           mMutex;

    mutable StaticAllocator<Max(sizeof(Storage::InstanceData), sizeof(sm::launcher::InstanceData),
                                sizeof(Storage::LayerData))
        + Max(sizeof(Storage::ServiceData), sizeof(sm::servicemanager::ServiceData))
        + Max(sizeof(Storage::LayerData), sizeof(sm::layermanager::LayerData))
        + Max(sizeof(Storage::CertInfo), sizeof(iam::certhandler::CertInfo))>
        mAllocator;
};

} // namespace aos::zephyr::storage

#endif
