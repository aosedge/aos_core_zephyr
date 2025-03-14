/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <aos/common/tools/fs.hpp>

#include "log.hpp"
#include "storage.hpp"

namespace aos::zephyr::storage {

/***********************************************************************************************************************
 * Public
 **********************************************************************************************************************/

Error Storage::Init()
{
    LOG_DBG() << "Initialize storage: " << cStoragePath;

    if (auto err = FS::MakeDirAll(cStoragePath); !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    auto instancePath = FS::JoinPath(cStoragePath, "instance.db");

    if (auto err = mInstanceDatabase.Init(instancePath); !err.IsNone()) {
        return err;
    }

    auto servicePath = FS::JoinPath(cStoragePath, "service.db");

    if (auto err = mServiceDatabase.Init(servicePath); !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    auto layerPath = FS::JoinPath(cStoragePath, "layer.db");

    if (auto err = mLayerDatabase.Init(layerPath); !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    auto certPath = FS::JoinPath(cStoragePath, "cert.db");

    if (auto err = mCertDatabase.Init(certPath); !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    return ErrorEnum::eNone;
}

Error Storage::AddInstance(const sm::launcher::InstanceData& instance)
{
    LockGuard lock(mMutex);

    LOG_DBG() << "Add instance: id=" << instance.mInstanceID;

    auto storageInstance = ConvertInstanceData(instance);

    return mInstanceDatabase.Add(
        *storageInstance, [](const Storage::InstanceData& storedInstance, const Storage::InstanceData& addedInstance) {
            return strcmp(storedInstance.mInstanceID, addedInstance.mInstanceID) == 0;
        });
}

Error Storage::UpdateInstance(const sm::launcher::InstanceData& instance)
{
    LockGuard lock(mMutex);

    LOG_DBG() << "Update instance: id=" << instance.mInstanceID;

    auto storageInstance = ConvertInstanceData(instance);

    return mInstanceDatabase.Update(*storageInstance,
        [&instance](const Storage::InstanceData& data) { return data.mInstanceID == instance.mInstanceID; });

    return ErrorEnum::eNone;
}

Error Storage::RemoveInstance(const String& instanceID)
{
    LockGuard lock(mMutex);

    LOG_DBG() << "Remove instance: id=" << instanceID;

    return mInstanceDatabase.Remove(
        [&instanceID](const Storage::InstanceData& data) { return data.mInstanceID == instanceID; });
}

Error Storage::GetAllInstances(Array<sm::launcher::InstanceData>& instances)
{
    LockGuard lock(mMutex);

    LOG_DBG() << "Get all instances";

    return mInstanceDatabase.ReadRecords([&instances, this](const Storage::InstanceData& storageInstance) -> Error {
        if (auto err = instances.EmplaceBack(); !err.IsNone()) {
            return AOS_ERROR_WRAP(err);
        }

        if (auto err = ConvertInstanceData(storageInstance, instances.Back()); !err.IsNone()) {
            return AOS_ERROR_WRAP(err);
        }

        return ErrorEnum::eNone;
    });
}

RetWithError<uint64_t> Storage::GetOperationVersion() const
{
    return 0;
}

Error Storage::SetOperationVersion(uint64_t version)
{
    (void)version;

    return ErrorEnum::eNone;
}

Error Storage::GetOverrideEnvVars(Array<cloudprotocol::EnvVarsInstanceInfo>& envVarsInstanceInfos) const
{
    (void)envVarsInstanceInfos;

    return ErrorEnum::eNone;
}

Error Storage::SetOverrideEnvVars(const Array<cloudprotocol::EnvVarsInstanceInfo>& envVarsInstanceInfos)
{
    (void)envVarsInstanceInfos;

    return ErrorEnum::eNone;
};

RetWithError<Time> Storage::GetOnlineTime() const
{
    return Time::Now();
}

Error Storage::SetOnlineTime(const Time& time)
{
    (void)time;

    return ErrorEnum::eNone;
}

Error Storage::AddService(const sm::servicemanager::ServiceData& service)
{
    LockGuard lock(mMutex);

    LOG_DBG() << "Add service: id=" << service.mServiceID << ", version=" << service.mVersion;

    auto storageService = ConvertServiceData(service);

    return mServiceDatabase.Add(
        *storageService, [](const Storage::ServiceData& storedService, const Storage::ServiceData& addedService) {
            return strcmp(storedService.mServiceID, addedService.mServiceID) == 0
                && strcmp(storedService.mVersion, addedService.mVersion) == 0;
        });
}

Error Storage::GetServiceVersions(const String& serviceID, Array<sm::servicemanager::ServiceData>& services)
{
    LockGuard lock(mMutex);

    LOG_DBG() << "Get service versions: id=" << serviceID;

    return mServiceDatabase.ReadRecords(
        [&services, &serviceID, this](const Storage::ServiceData& storageService) -> Error {
            if (storageService.mServiceID != serviceID) {
                return ErrorEnum::eNone;
            }

            if (auto err = services.EmplaceBack(); !err.IsNone()) {
                return AOS_ERROR_WRAP(err);
            }

            if (auto err = ConvertServiceData(storageService, services.Back()); !err.IsNone()) {
                return AOS_ERROR_WRAP(err);
            }

            return ErrorEnum::eNone;
        });
}

Error Storage::UpdateService(const sm::servicemanager::ServiceData& service)
{
    LockGuard lock(mMutex);

    LOG_DBG() << "Update service: id=" << service.mServiceID << ", version=" << service.mVersion
              << ", state=" << service.mState;

    auto storageService = ConvertServiceData(service);

    return mServiceDatabase.Update(*storageService, [&service](const Storage::ServiceData& data) {
        return data.mServiceID == service.mServiceID && data.mVersion == service.mVersion;
    });
}

Error Storage::RemoveService(const String& serviceID, const String& version)
{
    LockGuard lock(mMutex);

    LOG_DBG() << "Remove service: id=" << serviceID << ", version=" << version;

    return mServiceDatabase.Remove([&serviceID, &version](const Storage::ServiceData& data) {
        return data.mServiceID == serviceID && data.mVersion == version;
    });
}

Error Storage::GetAllServices(Array<sm::servicemanager::ServiceData>& services)
{
    LockGuard lock(mMutex);

    LOG_DBG() << "Get all services";

    return mServiceDatabase.ReadRecords([&services, this](const Storage::ServiceData& storageData) -> Error {
        if (auto err = services.EmplaceBack(); !err.IsNone()) {
            return AOS_ERROR_WRAP(err);
        }

        if (auto err = ConvertServiceData(storageData, services.Back()); !err.IsNone()) {
            return AOS_ERROR_WRAP(err);
        }

        return ErrorEnum::eNone;
    });
}

Error Storage::AddLayer(const sm::layermanager::LayerData& layer)
{
    LockGuard lock(mMutex);

    LOG_DBG() << "Add layer: digest=" << layer.mLayerDigest;

    auto storageLayer = ConvertLayerData(layer);

    return mLayerDatabase.Add(
        *storageLayer, [&storageLayer](const Storage::LayerData& storedLayer, const Storage::LayerData& addedLayer) {
            return strcmp(storedLayer.mLayerDigest, addedLayer.mLayerDigest) == 0;
        });
}

Error Storage::RemoveLayer(const String& digest)
{
    LockGuard lock(mMutex);

    LOG_DBG() << "Remove layer: digest=" << digest;

    return mLayerDatabase.Remove([&digest](const Storage::LayerData& data) { return data.mLayerDigest == digest; });
}

Error Storage::GetAllLayers(Array<sm::layermanager::LayerData>& layers) const
{
    LockGuard lock(mMutex);

    LOG_DBG() << "Get all layers";

    return mLayerDatabase.ReadRecords([&layers, this](const Storage::LayerData& storageLayer) -> Error {
        if (auto err = layers.EmplaceBack(); !err.IsNone()) {
            return AOS_ERROR_WRAP(err);
        }

        if (auto err = ConvertLayerData(storageLayer, layers.Back()); !err.IsNone()) {
            return AOS_ERROR_WRAP(err);
        }

        return ErrorEnum::eNone;
    });

    return ErrorEnum::eNone;
}

Error Storage::GetLayer(const String& digest, sm::layermanager::LayerData& layer) const
{
    LockGuard lock(mMutex);

    LOG_DBG() << "Get layer: digest=" << digest;

    auto storageLayer = MakeUnique<Storage::LayerData>(&mAllocator);

    if (auto err = mLayerDatabase.ReadRecordByFilter(
            *storageLayer, [&digest](const Storage::LayerData& data) { return data.mLayerDigest == digest; });
        !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    if (auto err = ConvertLayerData(*storageLayer, layer); !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    return ErrorEnum::eNone;
}

Error Storage::UpdateLayer(const sm::layermanager::LayerData& layer)
{
    LockGuard lock(mMutex);

    LOG_DBG() << "Update layer: digest=" << layer.mLayerDigest;

    auto storageLayer = ConvertLayerData(layer);

    return mLayerDatabase.Update(
        *storageLayer, [&layer](const Storage::LayerData& data) { return data.mLayerDigest == layer.mLayerDigest; });
}

Error Storage::AddCertInfo(const String& certType, const iam::certhandler::CertInfo& certInfo)
{
    LockGuard lock(mMutex);

    LOG_DBG() << "Add cert info: " << certType;

    auto storageCertInfo = ConvertCertInfo(certType, certInfo);

    return mCertDatabase.Add(*storageCertInfo,
        [&storageCertInfo](const Storage::CertInfo& storedCertInfo, const Storage::CertInfo& addedCertInfo) {
            return strcmp(storedCertInfo.mCertType, addedCertInfo.mCertType) == 0
                && storedCertInfo.mIssuerSize == addedCertInfo.mIssuerSize
                && storedCertInfo.mSerialSize == addedCertInfo.mSerialSize
                && memcmp(storedCertInfo.mIssuer, addedCertInfo.mIssuer, storedCertInfo.mIssuerSize) == 0
                && memcmp(storedCertInfo.mSerial, addedCertInfo.mSerial, storedCertInfo.mSerialSize) == 0;
        });
}

Error Storage::RemoveCertInfo(const String& certType, const String& certURL)
{
    LockGuard lock(mMutex);

    LOG_DBG() << "Remove cert info: " << certType;

    return mCertDatabase.Remove([&certType, &certURL](const Storage::CertInfo& data) {
        return data.mCertType == certType && data.mCertURL == certURL;
    });
}

Error Storage::RemoveAllCertsInfo(const String& certType)
{
    LockGuard lock(mMutex);

    LOG_DBG() << "Remove all cert info: " << certType;

    auto err = mCertDatabase.Remove([&certType](const Storage::CertInfo& data) { return data.mCertType == certType; });
    if (!err.IsNone() && !err.Is(ErrorEnum::eNotFound)) {
        return err;
    }

    return ErrorEnum::eNone;
}

Error Storage::GetCertsInfo(const String& certType, Array<iam::certhandler::CertInfo>& certsInfo)
{
    LockGuard lock(mMutex);

    LOG_DBG() << "Get cert info: " << certType;

    auto err = mCertDatabase.ReadRecords([&certsInfo, &certType, this](const Storage::CertInfo& certInfo) -> Error {
        if (certInfo.mCertType == certType) {
            auto cert = ConvertCertInfo(certInfo);

            auto err = certsInfo.PushBack(*cert);
            if (!err.IsNone()) {
                return AOS_ERROR_WRAP(err);
            }
        }

        return ErrorEnum::eNone;
    });

    return err;
}

Error Storage::GetCertInfo(const Array<uint8_t>& issuer, const Array<uint8_t>& serial, iam::certhandler::CertInfo& cert)
{
    LockGuard lock(mMutex);

    LOG_DBG() << "Get cert info by issuer and serial";

    UniquePtr<Storage::CertInfo> certInfo = MakeUnique<Storage::CertInfo>(&mAllocator);

    auto err = mCertDatabase.ReadRecordByFilter(*certInfo, [&issuer, &serial](const Storage::CertInfo& data) {
        Array<uint8_t> issuerArray(data.mIssuer, data.mIssuerSize);
        Array<uint8_t> serialArray(data.mSerial, data.mSerialSize);

        return issuerArray == issuer && serialArray == serial;
    });

    if (!err.IsNone()) {
        return err;
    }

    auto retCertInfo = ConvertCertInfo(*certInfo);

    cert = *retCertInfo;

    return ErrorEnum::eNone;
}

/***********************************************************************************************************************
 * Private
 **********************************************************************************************************************/

UniquePtr<Storage::InstanceData> Storage::ConvertInstanceData(const sm::launcher::InstanceData& instance)
{
    auto instanceInfo = MakeUnique<Storage::InstanceData>(&mAllocator);

    strcpy(instanceInfo->mInstanceID, instance.mInstanceID.CStr());

    instanceInfo->mInstanceIdent.mInstance = instance.mInstanceInfo.mInstanceIdent.mInstance;
    strcpy(instanceInfo->mInstanceIdent.mSubjectID, instance.mInstanceInfo.mInstanceIdent.mSubjectID.CStr());
    strcpy(instanceInfo->mInstanceIdent.mServiceID, instance.mInstanceInfo.mInstanceIdent.mServiceID.CStr());

    instanceInfo->mPriority = instance.mInstanceInfo.mPriority;
    strcpy(instanceInfo->mStatePath, instance.mInstanceInfo.mStatePath.CStr());
    strcpy(instanceInfo->mStoragePath, instance.mInstanceInfo.mStoragePath.CStr());
    instanceInfo->mUID = instance.mInstanceInfo.mUID;

    return instanceInfo;
}

Error Storage::ConvertInstanceData(const Storage::InstanceData& dbInstance, sm::launcher::InstanceData& outInstance)
{
    if (auto err = outInstance.mInstanceID.Assign(dbInstance.mInstanceID); !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    outInstance.mInstanceInfo.mInstanceIdent.mInstance  = dbInstance.mInstanceIdent.mInstance;
    outInstance.mInstanceInfo.mInstanceIdent.mSubjectID = dbInstance.mInstanceIdent.mSubjectID;
    outInstance.mInstanceInfo.mInstanceIdent.mServiceID = dbInstance.mInstanceIdent.mServiceID;
    outInstance.mInstanceInfo.mPriority                 = dbInstance.mPriority;
    outInstance.mInstanceInfo.mStatePath                = dbInstance.mStatePath;
    outInstance.mInstanceInfo.mStoragePath              = dbInstance.mStoragePath;
    outInstance.mInstanceInfo.mUID                      = dbInstance.mUID;

    return ErrorEnum::eNone;
}

UniquePtr<Storage::ServiceData> Storage::ConvertServiceData(const sm::servicemanager::ServiceData& service)
{
    auto serviceData = MakeUnique<ServiceData>(&mAllocator);

    strcpy(serviceData->mServiceID, service.mServiceID.CStr());
    strcpy(serviceData->mProviderID, service.mProviderID.CStr());
    strcpy(serviceData->mVersion, service.mVersion.CStr());
    strcpy(serviceData->mImagePath, service.mImagePath.CStr());
    strcpy(serviceData->mManifestDigest, service.mManifestDigest.CStr());
    serviceData->mTimestamp = service.mTimestamp.UnixTime();
    strcpy(serviceData->mState, service.mState.ToString().CStr());
    serviceData->mSize = service.mSize;
    serviceData->mGID  = service.mGID;

    return serviceData;
}

Error Storage::ConvertServiceData(
    const Storage::ServiceData& storageService, sm::servicemanager::ServiceData& outService)
{
    outService.mServiceID      = storageService.mServiceID;
    outService.mProviderID     = storageService.mProviderID;
    outService.mVersion        = storageService.mVersion;
    outService.mImagePath      = storageService.mImagePath;
    outService.mManifestDigest = storageService.mManifestDigest;
    outService.mTimestamp      = Time::Unix(storageService.mTimestamp.tv_sec, storageService.mTimestamp.tv_nsec);
    outService.mSize           = storageService.mSize;
    outService.mGID            = storageService.mGID;

    if (auto err = outService.mState.FromString(storageService.mState); !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    return ErrorEnum::eNone;
}

UniquePtr<Storage::LayerData> Storage::ConvertLayerData(const sm::layermanager::LayerData& layer)
{
    auto layerData = MakeUnique<LayerData>(&mAllocator);

    strcpy(layerData->mLayerDigest, layer.mLayerDigest.CStr());
    strcpy(layerData->mUnpackedLayerDigest, layer.mUnpackedLayerDigest.CStr());
    strcpy(layerData->mLayerID, layer.mLayerID.CStr());
    strcpy(layerData->mVersion, layer.mVersion.CStr());
    strcpy(layerData->mPath, layer.mPath.CStr());
    strcpy(layerData->mOSVersion, layer.mOSVersion.CStr());
    layerData->mTimestamp = layer.mTimestamp.UnixTime();
    strcpy(layerData->mState, layer.mState.ToString().CStr());
    layerData->mSize = layer.mSize;

    return layerData;
}

Error Storage::ConvertLayerData(const Storage::LayerData& dbLayer, sm::layermanager::LayerData& outLayer) const
{
    outLayer.mLayerDigest         = dbLayer.mLayerDigest;
    outLayer.mUnpackedLayerDigest = dbLayer.mUnpackedLayerDigest;
    outLayer.mLayerID             = dbLayer.mLayerID;
    outLayer.mVersion             = dbLayer.mVersion;
    outLayer.mPath                = dbLayer.mPath;
    outLayer.mOSVersion           = dbLayer.mOSVersion;
    outLayer.mTimestamp           = Time::Unix(dbLayer.mTimestamp.tv_sec, dbLayer.mTimestamp.tv_nsec);
    outLayer.mSize                = dbLayer.mSize;

    if (auto err = outLayer.mState.FromString(dbLayer.mState); !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    return ErrorEnum::eNone;
}

UniquePtr<Storage::CertInfo> Storage::ConvertCertInfo(
    const String& certType, const iam::certhandler::CertInfo& certInfo)
{
    UniquePtr<Storage::CertInfo> cert = MakeUnique<Storage::CertInfo>(&mAllocator);

    memcpy(cert->mIssuer, certInfo.mIssuer.Get(), certInfo.mIssuer.Size());
    memcpy(cert->mSerial, certInfo.mSerial.Get(), certInfo.mSerial.Size());
    strcpy(cert->mCertURL, certInfo.mCertURL.CStr());
    strcpy(cert->mKeyURL, certInfo.mKeyURL.CStr());
    strcpy(cert->mCertType, certType.CStr());
    cert->mNotAfter   = certInfo.mNotAfter.UnixTime();
    cert->mIssuerSize = certInfo.mIssuer.Size();
    cert->mSerialSize = certInfo.mSerial.Size();

    return cert;
}

UniquePtr<iam::certhandler::CertInfo> Storage::ConvertCertInfo(const Storage::CertInfo& certInfo)
{
    UniquePtr<iam::certhandler::CertInfo> cert = MakeUnique<iam::certhandler::CertInfo>(&mAllocator);

    cert->mIssuer   = Array<uint8_t>(certInfo.mIssuer, certInfo.mIssuerSize);
    cert->mSerial   = Array<uint8_t>(certInfo.mSerial, certInfo.mSerialSize);
    cert->mCertURL  = certInfo.mCertURL;
    cert->mKeyURL   = certInfo.mKeyURL;
    cert->mNotAfter = Time::Unix(certInfo.mNotAfter.tv_sec, certInfo.mNotAfter.tv_nsec);

    return cert;
}

} // namespace aos::zephyr::storage
