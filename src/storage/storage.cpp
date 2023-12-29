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

    return aos::ErrorEnum::eNone;
}

aos::Error Storage::AddInstance(const aos::InstanceInfo& instance)
{
    aos::LockGuard lock(mMutex);

    LOG_DBG() << "Add instance: " << instance.mInstanceIdent;

    return mInstanceDatabase.Add(ConvertInstanceInfo(instance));
}

aos::Error Storage::UpdateInstance(const aos::InstanceInfo& instance)
{
    aos::LockGuard lock(mMutex);

    LOG_DBG() << "Update instance: " << instance.mInstanceIdent;

    return mInstanceDatabase.Update(ConvertInstanceInfo(instance), [&instance](const InstanceInfo& data) {
        return data.mInstanceIdent.mInstance == instance.mInstanceIdent.mInstance
            && strcmp(data.mInstanceIdent.mSubjectID, instance.mInstanceIdent.mSubjectID.CStr()) == 0
            && strcmp(data.mInstanceIdent.mServiceID, instance.mInstanceIdent.mServiceID.CStr()) == 0;
    });

    return aos::ErrorEnum::eNone;
}

aos::Error Storage::RemoveInstance(const aos::InstanceIdent& instanceIdent)
{
    aos::LockGuard lock(mMutex);

    LOG_DBG() << "Remove instance: " << instanceIdent;

    return mInstanceDatabase.Remove([&instanceIdent](const InstanceInfo& data) {
        return data.mInstanceIdent.mInstance == instanceIdent.mInstance
            && strcmp(data.mInstanceIdent.mSubjectID, instanceIdent.mSubjectID.CStr()) == 0
            && strcmp(data.mInstanceIdent.mServiceID, instanceIdent.mServiceID.CStr()) == 0;
    });

    return aos::ErrorEnum::eNone;
}

aos::Error Storage::GetAllInstances(aos::Array<aos::InstanceInfo>& instances)
{
    aos::LockGuard lock(mMutex);

    LOG_DBG() << "Get all instances";

    auto err = mInstanceDatabase.ReadRecords([&instances, this](const InstanceInfo& instanceInfo) -> aos::Error {
        auto instance = ConvertInstanceInfo(instanceInfo);
        auto err      = instances.PushBack(instance);
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

    return mServiceDatabase.Add(ConvertServiceData(service));
}

aos::Error Storage::UpdateService(const aos::sm::servicemanager::ServiceData& service)
{
    aos::LockGuard lock(mMutex);

    LOG_DBG() << "Update service: " << service.mServiceID;

    return mServiceDatabase.Update(ConvertServiceData(service),
        [&service](const ServiceData& data) { return strcmp(data.mServiceID, service.mServiceID.CStr()) == 0; });
}

aos::Error Storage::RemoveService(const aos::String& serviceID, uint64_t aosVersion)
{
    aos::LockGuard lock(mMutex);

    LOG_DBG() << "Remove service: " << serviceID;

    return mServiceDatabase.Remove([&serviceID, aosVersion](const ServiceData& data) {
        return strcmp(data.mServiceID, serviceID.CStr()) == 0 && data.mVersionInfo.mAosVersion == aosVersion;
    });
}

aos::Error Storage::GetAllServices(aos::Array<aos::sm::servicemanager::ServiceData>& services)
{
    aos::LockGuard lock(mMutex);

    LOG_DBG() << "Get all services";

    auto err = mServiceDatabase.ReadRecords([&services, this](const ServiceData& serviceData) -> aos::Error {
        auto service = ConvertServiceData(serviceData);

        auto err = services.PushBack(service);
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

    ServiceData serviceData;

    auto err = mServiceDatabase.ReadRecordByFilter(
        serviceData, [&serviceID](const ServiceData& data) { return strcmp(data.mServiceID, serviceID.CStr()) == 0; });

    if (!err.IsNone()) {
        return aos::RetWithError<aos::sm::servicemanager::ServiceData>(aos::sm::servicemanager::ServiceData {}, err);
    }

    return aos::RetWithError<aos::sm::servicemanager::ServiceData>(ConvertServiceData(serviceData));
}

/***********************************************************************************************************************
 * Private
 **********************************************************************************************************************/

Storage::InstanceInfo Storage::ConvertInstanceInfo(const aos::InstanceInfo& instance)
{
    InstanceInfo instanceInfo;

    instanceInfo.mInstanceIdent.mInstance = instance.mInstanceIdent.mInstance;
    strcpy(instanceInfo.mInstanceIdent.mSubjectID, instance.mInstanceIdent.mSubjectID.CStr());
    strcpy(instanceInfo.mInstanceIdent.mServiceID, instance.mInstanceIdent.mServiceID.CStr());
    instanceInfo.mPriority = instance.mPriority;
    strcpy(instanceInfo.mStatePath, instance.mStatePath.CStr());
    strcpy(instanceInfo.mStoragePath, instance.mStoragePath.CStr());
    instanceInfo.mUID = instance.mUID;

    return instanceInfo;
}

aos::InstanceInfo Storage::ConvertInstanceInfo(const InstanceInfo& instance)
{
    aos::InstanceInfo instanceInfo;

    instanceInfo.mInstanceIdent.mInstance  = instance.mInstanceIdent.mInstance;
    instanceInfo.mInstanceIdent.mSubjectID = instance.mInstanceIdent.mSubjectID;
    instanceInfo.mInstanceIdent.mServiceID = instance.mInstanceIdent.mServiceID;
    instanceInfo.mPriority                 = instance.mPriority;
    instanceInfo.mStatePath                = instance.mStatePath;
    instanceInfo.mStoragePath              = instance.mStoragePath;
    instanceInfo.mUID                      = instance.mUID;

    return instanceInfo;
}

Storage::ServiceData Storage::ConvertServiceData(const aos::sm::servicemanager::ServiceData& service)
{
    ServiceData serviceData;

    serviceData.mVersionInfo.mAosVersion = service.mVersionInfo.mAosVersion;
    strcpy(serviceData.mVersionInfo.mVendorVersion, service.mVersionInfo.mVendorVersion.CStr());
    strcpy(serviceData.mVersionInfo.mDescription, service.mVersionInfo.mDescription.CStr());
    strcpy(serviceData.mServiceID, service.mServiceID.CStr());
    strcpy(serviceData.mProviderID, service.mProviderID.CStr());
    strcpy(serviceData.mImagePath, service.mImagePath.CStr());

    return serviceData;
}

aos::sm::servicemanager::ServiceData Storage::ConvertServiceData(const Storage::ServiceData& service)
{
    aos::sm::servicemanager::ServiceData serviceData;

    serviceData.mVersionInfo.mAosVersion    = service.mVersionInfo.mAosVersion;
    serviceData.mVersionInfo.mVendorVersion = service.mVersionInfo.mVendorVersion;
    serviceData.mVersionInfo.mDescription   = service.mVersionInfo.mDescription;
    serviceData.mServiceID                  = service.mServiceID;
    serviceData.mProviderID                 = service.mProviderID;
    serviceData.mImagePath                  = service.mImagePath;

    return serviceData;
}
