/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <aos/common/tools/fs.hpp>

#include "log.hpp"
#include "storage.hpp"

/***********************************************************************************************************************
 * Public
 **********************************************************************************************************************/

aos::Error Storage::Init()
{
    LOG_DBG() << "Initialize storage: " << cStoragePath;

    aos::Error err;

    if (!(err = aos::FS::MakeDirAll(cStoragePath)).IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    auto instancePath = aos::FS::JoinPath(cStoragePath, "instance.db");

    if (!(err = mInstanceDatabase.Init(instancePath)).IsNone()) {
        return err;
    }

    auto servicePath = aos::FS::JoinPath(cStoragePath, "service.db");

    if (!(err = mServiceDatabase.Init(servicePath)).IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    auto certPath = aos::FS::JoinPath(cStoragePath, "cert.db");
    if (!(err = mCertDatabase.Init(certPath)).IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    return aos::ErrorEnum::eNone;
}

aos::Error Storage::AddInstance(const aos::InstanceInfo& instance)
{
    aos::LockGuard lock(mMutex);

    LOG_DBG() << "Add instance: " << instance.mInstanceIdent;

    auto storageInstance = ConvertInstanceInfo(instance);

    return mInstanceDatabase.Add(*storageInstance);
}

aos::Error Storage::UpdateInstance(const aos::InstanceInfo& instance)
{
    aos::LockGuard lock(mMutex);

    LOG_DBG() << "Update instance: " << instance.mInstanceIdent;

    auto storageInstance = ConvertInstanceInfo(instance);

    return mInstanceDatabase.Update(*storageInstance, [&instance](const InstanceInfo& data) {
        return data.mInstanceIdent.mInstance == instance.mInstanceIdent.mInstance
            && data.mInstanceIdent.mSubjectID == instance.mInstanceIdent.mSubjectID
            && data.mInstanceIdent.mServiceID == instance.mInstanceIdent.mServiceID;
    });

    return aos::ErrorEnum::eNone;
}

aos::Error Storage::RemoveInstance(const aos::InstanceIdent& instanceIdent)
{
    aos::LockGuard lock(mMutex);

    LOG_DBG() << "Remove instance: " << instanceIdent;

    return mInstanceDatabase.Remove([&instanceIdent](const InstanceInfo& data) {
        return data.mInstanceIdent.mInstance == instanceIdent.mInstance
            && data.mInstanceIdent.mSubjectID == instanceIdent.mSubjectID
            && data.mInstanceIdent.mServiceID == instanceIdent.mServiceID;
    });

    return aos::ErrorEnum::eNone;
}

aos::Error Storage::GetAllInstances(aos::Array<aos::InstanceInfo>& instances)
{
    aos::LockGuard lock(mMutex);

    LOG_DBG() << "Get all instances";

    auto err = mInstanceDatabase.ReadRecords([&instances, this](const InstanceInfo& instanceInfo) -> aos::Error {
        auto instance = ConvertInstanceInfo(instanceInfo);

        auto err = instances.PushBack(*instance);
        if (!err.IsNone()) {
            return AOS_ERROR_WRAP(err);
        }

        return aos::ErrorEnum::eNone;
    });

    return err;
}

aos::Error Storage::AddService(const aos::sm::servicemanager::ServiceData& service)
{
    aos::LockGuard lock(mMutex);

    LOG_DBG() << "Add service: " << service.mServiceID;

    auto storageService = ConvertServiceData(service);

    return mServiceDatabase.Add(*storageService);
}

aos::Error Storage::UpdateService(const aos::sm::servicemanager::ServiceData& service)
{
    aos::LockGuard lock(mMutex);

    LOG_DBG() << "Update service: " << service.mServiceID;

    auto storageService = ConvertServiceData(service);

    return mServiceDatabase.Update(
        *storageService, [&service](const ServiceData& data) { return data.mServiceID == service.mServiceID; });
}

aos::Error Storage::RemoveService(const aos::String& serviceID, uint64_t aosVersion)
{
    aos::LockGuard lock(mMutex);

    LOG_DBG() << "Remove service: " << serviceID;

    return mServiceDatabase.Remove([&serviceID, aosVersion](const ServiceData& data) {
        return data.mServiceID == serviceID && data.mVersionInfo.mAosVersion == aosVersion;
    });
}

aos::Error Storage::GetAllServices(aos::Array<aos::sm::servicemanager::ServiceData>& services)
{
    aos::LockGuard lock(mMutex);

    LOG_DBG() << "Get all services";

    auto err = mServiceDatabase.ReadRecords([&services, this](const ServiceData& serviceData) -> aos::Error {
        auto service = ConvertServiceData(serviceData);

        auto err = services.PushBack(*service);
        if (!err.IsNone()) {
            return AOS_ERROR_WRAP(err);
        }

        return aos::ErrorEnum::eNone;
    });

    return err;
}

aos::RetWithError<aos::sm::servicemanager::ServiceData> Storage::GetService(const aos::String& serviceID)
{
    aos::LockGuard lock(mMutex);

    LOG_DBG() << "Get service: " << serviceID;

    aos::UniquePtr<ServiceData> serviceData = aos::MakeUnique<ServiceData>(&mAllocator);

    auto err = mServiceDatabase.ReadRecordByFilter(
        *serviceData, [&serviceID](const ServiceData& data) { return data.mServiceID == serviceID; });

    if (!err.IsNone()) {
        return aos::RetWithError<aos::sm::servicemanager::ServiceData>(aos::sm::servicemanager::ServiceData {}, err);
    }

    auto retServiceData = ConvertServiceData(*serviceData);

    return aos::RetWithError<aos::sm::servicemanager::ServiceData>(*retServiceData);
}

aos::Error Storage::AddCertInfo(const aos::String& certType, const aos::iam::certhandler::CertInfo& certInfo)
{
    aos::LockGuard lock(mMutex);

    LOG_DBG() << "Add cert info: " << certType;

    auto storageCertInfo = ConvertCertInfo(certType, certInfo);

    return mCertDatabase.Add(*storageCertInfo);
}

aos::Error Storage::RemoveCertInfo(const aos::String& certType, const aos::String& certURL)
{
    aos::LockGuard lock(mMutex);

    LOG_DBG() << "Remove cert info: " << certType;

    return mCertDatabase.Remove(
        [&certType, &certURL](const CertInfo& data) { return data.mCertType == certType && data.mCertURL == certURL; });
}

aos::Error Storage::RemoveAllCertsInfo(const aos::String& certType)
{
    aos::LockGuard lock(mMutex);

    LOG_DBG() << "Remove all cert info: " << certType;

    auto err = mCertDatabase.Remove([&certType](const CertInfo& data) { return data.mCertType == certType; });
    if (!err.IsNone() && !err.Is(aos::ErrorEnum::eNotFound)) {
        return err;
    }

    return aos::ErrorEnum::eNone;
}

aos::Error Storage::GetCertsInfo(const aos::String& certType, aos::Array<aos::iam::certhandler::CertInfo>& certsInfo)
{
    aos::LockGuard lock(mMutex);

    LOG_DBG() << "Get cert info: " << certType;

    auto err = mCertDatabase.ReadRecords([&certsInfo, &certType, this](const CertInfo& certInfo) -> aos::Error {
        if (certInfo.mCertType == certType) {
            auto cert = ConvertCertInfo(certInfo);

            auto err = certsInfo.PushBack(*cert);
            if (!err.IsNone()) {
                return AOS_ERROR_WRAP(err);
            }
        }

        return aos::ErrorEnum::eNone;
    });

    return err;
}

aos::Error Storage::GetCertInfo(
    const aos::Array<uint8_t>& issuer, const aos::Array<uint8_t>& serial, aos::iam::certhandler::CertInfo& cert)
{
    aos::LockGuard lock(mMutex);

    LOG_DBG() << "Get cert info by issuer and serial";

    aos::UniquePtr<CertInfo> certInfo = aos::MakeUnique<CertInfo>(&mAllocator);

    auto err = mCertDatabase.ReadRecordByFilter(*certInfo, [&issuer, &serial](const CertInfo& data) {
        aos::Array<uint8_t> issuerArray(data.mIssuer, data.mIssuerSize);
        aos::Array<uint8_t> serialArray(data.mSerial, data.mSerialSize);

        return issuerArray == issuer && serialArray == serial;
    });

    if (!err.IsNone()) {
        return err;
    }

    auto retCertInfo = ConvertCertInfo(*certInfo);

    cert = *retCertInfo;

    return aos::ErrorEnum::eNone;
}

/***********************************************************************************************************************
 * Private
 **********************************************************************************************************************/

aos::UniquePtr<Storage::InstanceInfo> Storage::ConvertInstanceInfo(const aos::InstanceInfo& instance)
{
    aos::UniquePtr<InstanceInfo> instanceInfo = aos::MakeUnique<InstanceInfo>(&mAllocator);

    instanceInfo->mInstanceIdent.mInstance = instance.mInstanceIdent.mInstance;
    strcpy(instanceInfo->mInstanceIdent.mSubjectID, instance.mInstanceIdent.mSubjectID.CStr());
    strcpy(instanceInfo->mInstanceIdent.mServiceID, instance.mInstanceIdent.mServiceID.CStr());
    instanceInfo->mPriority = instance.mPriority;
    strcpy(instanceInfo->mStatePath, instance.mStatePath.CStr());
    strcpy(instanceInfo->mStoragePath, instance.mStoragePath.CStr());
    instanceInfo->mUID = instance.mUID;

    return instanceInfo;
}

aos::UniquePtr<aos::InstanceInfo> Storage::ConvertInstanceInfo(const InstanceInfo& instance)
{
    aos::UniquePtr<aos::InstanceInfo> instanceInfo = aos::MakeUnique<aos::InstanceInfo>(&mAllocator);

    instanceInfo->mInstanceIdent.mInstance  = instance.mInstanceIdent.mInstance;
    instanceInfo->mInstanceIdent.mSubjectID = instance.mInstanceIdent.mSubjectID;
    instanceInfo->mInstanceIdent.mServiceID = instance.mInstanceIdent.mServiceID;
    instanceInfo->mPriority                 = instance.mPriority;
    instanceInfo->mStatePath                = instance.mStatePath;
    instanceInfo->mStoragePath              = instance.mStoragePath;
    instanceInfo->mUID                      = instance.mUID;

    return instanceInfo;
}

aos::UniquePtr<Storage::ServiceData> Storage::ConvertServiceData(const aos::sm::servicemanager::ServiceData& service)
{
    aos::UniquePtr<ServiceData> serviceData = aos::MakeUnique<ServiceData>(&mAllocator);

    serviceData->mVersionInfo.mAosVersion = service.mVersionInfo.mAosVersion;
    strcpy(serviceData->mVersionInfo.mVendorVersion, service.mVersionInfo.mVendorVersion.CStr());
    strcpy(serviceData->mVersionInfo.mDescription, service.mVersionInfo.mDescription.CStr());
    strcpy(serviceData->mServiceID, service.mServiceID.CStr());
    strcpy(serviceData->mProviderID, service.mProviderID.CStr());
    strcpy(serviceData->mImagePath, service.mImagePath.CStr());

    return serviceData;
}

aos::UniquePtr<aos::sm::servicemanager::ServiceData> Storage::ConvertServiceData(const Storage::ServiceData& service)
{
    aos::UniquePtr<aos::sm::servicemanager::ServiceData> serviceData
        = aos::MakeUnique<aos::sm::servicemanager::ServiceData>(&mAllocator);

    serviceData->mVersionInfo.mAosVersion    = service.mVersionInfo.mAosVersion;
    serviceData->mVersionInfo.mVendorVersion = service.mVersionInfo.mVendorVersion;
    serviceData->mVersionInfo.mDescription   = service.mVersionInfo.mDescription;
    serviceData->mServiceID                  = service.mServiceID;
    serviceData->mProviderID                 = service.mProviderID;
    serviceData->mImagePath                  = service.mImagePath;

    return serviceData;
}

aos::UniquePtr<Storage::CertInfo> Storage::ConvertCertInfo(
    const aos::String& certType, const aos::iam::certhandler::CertInfo& certInfo)
{
    aos::UniquePtr<CertInfo> cert = aos::MakeUnique<CertInfo>(&mAllocator);

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

aos::UniquePtr<aos::iam::certhandler::CertInfo> Storage::ConvertCertInfo(const Storage::CertInfo& certInfo)
{
    aos::UniquePtr<aos::iam::certhandler::CertInfo> cert
        = aos::MakeUnique<aos::iam::certhandler::CertInfo>(&mAllocator);

    cert->mIssuer   = aos::Array<uint8_t>(certInfo.mIssuer, certInfo.mIssuerSize);
    cert->mSerial   = aos::Array<uint8_t>(certInfo.mSerial, certInfo.mSerialSize);
    cert->mCertURL  = certInfo.mCertURL;
    cert->mKeyURL   = certInfo.mKeyURL;
    cert->mNotAfter = aos::Time::Unix(certInfo.mNotAfter.tv_sec, certInfo.mNotAfter.tv_nsec);

    return cert;
}
