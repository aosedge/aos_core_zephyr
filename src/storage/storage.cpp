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

    auto certPath = FS::JoinPath(cStoragePath, "cert.db");
    if (auto err = mCertDatabase.Init(certPath); !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    return ErrorEnum::eNone;
}

// cppcheck-suppress unusedFunction
Error Storage::AddInstance(const aos::InstanceInfo& instance)
{
    LockGuard lock(mMutex);

    LOG_DBG() << "Add instance: " << instance.mInstanceIdent;

    auto storageInstance = ConvertInstanceInfo(instance);

    return mInstanceDatabase.Add(
        *storageInstance, [](const Storage::InstanceInfo& storedInstance, const Storage::InstanceInfo& addedInstance) {
            return storedInstance.mInstanceIdent == addedInstance.mInstanceIdent;
        });
}

// cppcheck-suppress unusedFunction
Error Storage::UpdateInstance(const aos::InstanceInfo& instance)
{
    LockGuard lock(mMutex);

    LOG_DBG() << "Update instance: " << instance.mInstanceIdent;

    auto storageInstance = ConvertInstanceInfo(instance);

    return mInstanceDatabase.Update(*storageInstance, [&instance](const Storage::InstanceInfo& data) {
        return data.mInstanceIdent.mInstance == instance.mInstanceIdent.mInstance
            && data.mInstanceIdent.mSubjectID == instance.mInstanceIdent.mSubjectID
            && data.mInstanceIdent.mServiceID == instance.mInstanceIdent.mServiceID;
    });

    return ErrorEnum::eNone;
}

// cppcheck-suppress unusedFunction
Error Storage::RemoveInstance(const aos::InstanceIdent& instanceIdent)
{
    LockGuard lock(mMutex);

    LOG_DBG() << "Remove instance: " << instanceIdent;

    return mInstanceDatabase.Remove([&instanceIdent](const Storage::InstanceInfo& data) {
        return data.mInstanceIdent.mInstance == instanceIdent.mInstance
            && data.mInstanceIdent.mSubjectID == instanceIdent.mSubjectID
            && data.mInstanceIdent.mServiceID == instanceIdent.mServiceID;
    });

    return ErrorEnum::eNone;
}

// cppcheck-suppress unusedFunction
Error Storage::GetAllInstances(Array<aos::InstanceInfo>& instances)
{
    LockGuard lock(mMutex);

    LOG_DBG() << "Get all instances";

    auto err = mInstanceDatabase.ReadRecords([&instances, this](const Storage::InstanceInfo& instanceInfo) -> Error {
        auto instance = ConvertInstanceInfo(instanceInfo);

        auto err = instances.PushBack(*instance);
        if (!err.IsNone()) {
            return AOS_ERROR_WRAP(err);
        }

        return ErrorEnum::eNone;
    });

    return err;
}

// cppcheck-suppress unusedFunction
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

// cppcheck-suppress unusedFunction
Error Storage::UpdateService(const sm::servicemanager::ServiceData& service)
{
    LockGuard lock(mMutex);

    LOG_DBG() << "Update service: " << service.mServiceID;

    auto storageService = ConvertServiceData(service);

    return mServiceDatabase.Update(*storageService,
        [&service](const Storage::ServiceData& data) { return data.mServiceID == service.mServiceID; });
}

Error Storage::RemoveService(const String& serviceID, const String& version)
{
    LockGuard lock(mMutex);

    LOG_DBG() << "Remove service: id=" << serviceID << ", version=" << version;

    return mServiceDatabase.Remove([&serviceID, &version](const Storage::ServiceData& data) {
        return data.mServiceID == serviceID && data.mVersion == version;
    });
}

// cppcheck-suppress unusedFunction
Error Storage::GetAllServices(Array<sm::servicemanager::ServiceData>& services)
{
    LockGuard lock(mMutex);

    LOG_DBG() << "Get all services";

    auto err = mServiceDatabase.ReadRecords([&services, this](const Storage::ServiceData& serviceData) -> Error {
        auto service = ConvertServiceData(serviceData);

        auto err = services.PushBack(*service);
        if (!err.IsNone()) {
            return AOS_ERROR_WRAP(err);
        }

        return ErrorEnum::eNone;
    });

    return err;
}

// cppcheck-suppress unusedFunction
RetWithError<sm::servicemanager::ServiceData> Storage::GetService(const String& serviceID)
{
    LockGuard lock(mMutex);

    LOG_DBG() << "Get service: " << serviceID;

    UniquePtr<Storage::ServiceData> serviceData = MakeUnique<Storage::ServiceData>(&mAllocator);

    auto err = mServiceDatabase.ReadRecordByFilter(
        *serviceData, [&serviceID](const Storage::ServiceData data) { return data.mServiceID == serviceID; });

    if (!err.IsNone()) {
        return RetWithError<sm::servicemanager::ServiceData>(sm::servicemanager::ServiceData {}, err);
    }

    auto retServiceData = ConvertServiceData(*serviceData);

    return RetWithError<sm::servicemanager::ServiceData>(*retServiceData);
}

// cppcheck-suppress unusedFunction
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

// cppcheck-suppress unusedFunction
Error Storage::RemoveCertInfo(const String& certType, const String& certURL)
{
    LockGuard lock(mMutex);

    LOG_DBG() << "Remove cert info: " << certType;

    return mCertDatabase.Remove([&certType, &certURL](const Storage::CertInfo& data) {
        return data.mCertType == certType && data.mCertURL == certURL;
    });
}

// cppcheck-suppress unusedFunction
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

// cppcheck-suppress unusedFunction
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

// cppcheck-suppress unusedFunction
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

UniquePtr<Storage::InstanceInfo> Storage::ConvertInstanceInfo(const aos::InstanceInfo& instance)
{
    UniquePtr<Storage::InstanceInfo> instanceInfo = MakeUnique<Storage::InstanceInfo>(&mAllocator);

    instanceInfo->mInstanceIdent.mInstance = instance.mInstanceIdent.mInstance;
    strcpy(instanceInfo->mInstanceIdent.mSubjectID, instance.mInstanceIdent.mSubjectID.CStr());
    strcpy(instanceInfo->mInstanceIdent.mServiceID, instance.mInstanceIdent.mServiceID.CStr());
    instanceInfo->mPriority = instance.mPriority;
    strcpy(instanceInfo->mStatePath, instance.mStatePath.CStr());
    strcpy(instanceInfo->mStoragePath, instance.mStoragePath.CStr());
    instanceInfo->mUID = instance.mUID;

    return instanceInfo;
}

UniquePtr<aos::InstanceInfo> Storage::ConvertInstanceInfo(const Storage::InstanceInfo& instance)
{
    UniquePtr<aos::InstanceInfo> instanceInfo = MakeUnique<aos::InstanceInfo>(&mAllocator);

    instanceInfo->mInstanceIdent.mInstance  = instance.mInstanceIdent.mInstance;
    instanceInfo->mInstanceIdent.mSubjectID = instance.mInstanceIdent.mSubjectID;
    instanceInfo->mInstanceIdent.mServiceID = instance.mInstanceIdent.mServiceID;
    instanceInfo->mPriority                 = instance.mPriority;
    instanceInfo->mStatePath                = instance.mStatePath;
    instanceInfo->mStoragePath              = instance.mStoragePath;
    instanceInfo->mUID                      = instance.mUID;

    return instanceInfo;
}

UniquePtr<Storage::ServiceData> Storage::ConvertServiceData(const sm::servicemanager::ServiceData& service)
{
    UniquePtr<ServiceData> serviceData = MakeUnique<ServiceData>(&mAllocator);

    strcpy(serviceData->mServiceID, service.mServiceID.CStr());
    strcpy(serviceData->mProviderID, service.mProviderID.CStr());
    strcpy(serviceData->mVersion, service.mVersion.CStr());
    strcpy(serviceData->mImagePath, service.mImagePath.CStr());

    return serviceData;
}

UniquePtr<sm::servicemanager::ServiceData> Storage::ConvertServiceData(const Storage::ServiceData& service)
{
    UniquePtr<sm::servicemanager::ServiceData> serviceData = MakeUnique<sm::servicemanager::ServiceData>(&mAllocator);

    serviceData->mServiceID  = service.mServiceID;
    serviceData->mProviderID = service.mProviderID;
    serviceData->mVersion    = service.mVersion;
    serviceData->mImagePath  = service.mImagePath;

    return serviceData;
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
