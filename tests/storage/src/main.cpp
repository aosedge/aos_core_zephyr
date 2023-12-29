/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>

#include <aos/common/tools/log.hpp>
#include <aos/common/tools/string.hpp>
#include <aos/common/types.hpp>

#include "storage/storage.hpp"

ZTEST_SUITE(storage, NULL, NULL, NULL, NULL, NULL);

void TestLogCallback(aos::LogModule module, aos::LogLevel level, const aos::String& message)
{
    printk("[storage] %s \n", message.CStr());
}

ZTEST(storage, test_AddUpdateRemoveInstance)
{
    aos::Log::SetCallback(TestLogCallback);

    Storage storage;

    zassert_equal(storage.Init(), aos::ErrorEnum::eNone, "Failed to initialize storage");

    aos::StaticArray<aos::InstanceInfo, 2> instances;

    aos::InstanceInfo instanceInfo;
    instanceInfo.mInstanceIdent.mInstance  = 1;
    instanceInfo.mInstanceIdent.mServiceID = "service_id";
    instanceInfo.mInstanceIdent.mSubjectID = "subject_id";
    instanceInfo.mPriority                 = 1;
    instanceInfo.mStoragePath              = "storage_path";
    instanceInfo.mStatePath                = "state_path";
    instanceInfo.mUID                      = 1;

    instances.PushBack(instanceInfo);

    aos::InstanceInfo instanceInfo2;
    instanceInfo2.mInstanceIdent.mInstance  = 2;
    instanceInfo2.mInstanceIdent.mServiceID = "service_id";
    instanceInfo2.mInstanceIdent.mSubjectID = "subject_id";
    instanceInfo2.mPriority                 = 2;
    instanceInfo2.mStoragePath              = "storage_path";
    instanceInfo2.mStatePath                = "state_path";
    instanceInfo2.mUID                      = 2;

    instances.PushBack(instanceInfo2);

    for (const auto& instance : instances) {
        zassert_equal(storage.AddInstance(instance), aos::ErrorEnum::eNone, "Failed to add instance");
    }

    for (const auto& instance : instances) {
        zassert_equal(storage.AddInstance(instance), aos::ErrorEnum::eAlreadyExist, "Unexpected error");
    }

    aos::StaticArray<aos::InstanceInfo, 2> instances2;
    zassert_equal(storage.GetAllInstances(instances2), aos::ErrorEnum::eNone, "Failed to get all instances");

    zassert_equal(instances2.Size(), 2, "Unexpected number of instances");

    for (const auto& instance : instances2) {
        zassert_equal(instances.Find(instance).mError, aos::ErrorEnum::eNone, "Unexpected instance");
    }

    instances[0].mStoragePath = "storage_path2";
    instances[0].mStatePath   = "state_path2";

    instances[1].mStoragePath = "storage_path3";
    instances[1].mStatePath   = "state_path3";

    for (const auto& instance : instances) {
        zassert_equal(storage.UpdateInstance(instance), aos::ErrorEnum::eNone, "Failed to update instance");
    }

    instances2.Clear();
    zassert_equal(storage.GetAllInstances(instances2), aos::ErrorEnum::eNone, "Failed to get all instances");

    zassert_equal(instances2.Size(), 2, "Unexpected number of instances");

    for (const auto& instance : instances2) {
        zassert_equal(instances.Find(instance).mError, aos::ErrorEnum::eNone, "Unexpected instance");
    }

    for (const auto& instance : instances) {
        zassert_equal(
            storage.RemoveInstance(instance.mInstanceIdent), aos::ErrorEnum::eNone, "Failed to remove instance");
    }

    instances2.Clear();
    zassert_equal(storage.GetAllInstances(instances2), aos::ErrorEnum::eNone, "Failed to get all instances");

    zassert_equal(instances2.Size(), 0, "Unexpected number of instances");
}

ZTEST(storage, test_AddUpdateRemoveService)
{
    aos::Log::SetCallback(TestLogCallback);

    Storage storage;

    zassert_equal(storage.Init(), aos::ErrorEnum::eNone, "Failed to initialize storage");

    aos::StaticArray<aos::sm::servicemanager::ServiceData, 2> services;

    aos::sm::servicemanager::ServiceData serviceData;
    serviceData.mVersionInfo.mAosVersion    = 1;
    serviceData.mVersionInfo.mVendorVersion = "vendor_version";
    serviceData.mVersionInfo.mDescription   = "description";
    serviceData.mServiceID                  = "service_id";
    serviceData.mProviderID                 = "provider_id";
    serviceData.mImagePath                  = "image_path";

    services.PushBack(serviceData);

    aos::sm::servicemanager::ServiceData serviceData2;
    serviceData2.mVersionInfo.mAosVersion    = 2;
    serviceData2.mVersionInfo.mVendorVersion = "vendor_version2";
    serviceData2.mVersionInfo.mDescription   = "description2";
    serviceData2.mServiceID                  = "service_id2";
    serviceData2.mProviderID                 = "provider_id2";
    serviceData2.mImagePath                  = "image_path2";

    services.PushBack(serviceData2);

    for (const auto& service : services) {
        zassert_equal(storage.AddService(service), aos::ErrorEnum::eNone, "Failed to add service");
    }

    for (const auto& service : services) {
        zassert_equal(storage.AddService(service), aos::ErrorEnum::eAlreadyExist, "Unexpected error");
    }

    aos::StaticArray<aos::sm::servicemanager::ServiceData, 2> services2;
    zassert_equal(storage.GetAllServices(services2), aos::ErrorEnum::eNone, "Failed to get all services");

    zassert_equal(services2.Size(), 2, "Unexpected number of services");

    for (const auto& service : services2) {
        zassert_equal(services.Find(service).mError, aos::ErrorEnum::eNone, "Unexpected service");
    }

    aos::StaticString<aos::cServiceIDLen> serviceID {"service_id"};

    auto ret = storage.GetService(serviceID);

    zassert_equal(ret.mError, aos::ErrorEnum::eNone, "Failed to get service");
    zassert_equal(ret.mValue, serviceData, "Unexpected service");

    services[0].mImagePath = "image_path3";
    services[1].mImagePath = "image_path4";

    for (const auto& service : services) {
        zassert_equal(storage.UpdateService(service), aos::ErrorEnum::eNone, "Failed to update service");
    }

    services2.Clear();
    zassert_equal(storage.GetAllServices(services2), aos::ErrorEnum::eNone, "Failed to get all services");

    zassert_equal(services2.Size(), 2, "Unexpected number of services");

    for (const auto& service : services2) {
        zassert_equal(services.Find(service).mError, aos::ErrorEnum::eNone, "Unexpected service");
    }

    for (const auto& service : services) {
        zassert_equal(storage.RemoveService(service.mServiceID, service.mVersionInfo.mAosVersion),
            aos::ErrorEnum::eNone, "Failed to remove service");
    }

    services2.Clear();
    zassert_equal(storage.GetAllServices(services2), aos::ErrorEnum::eNone, "Failed to get all services");

    zassert_equal(services2.Size(), 0, "Unexpected number of services");
}
