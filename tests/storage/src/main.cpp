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

static const aos::Array<uint8_t> StringToDN(const char* str)
{
    return aos::Array<uint8_t>(reinterpret_cast<const uint8_t*>(str), strlen(str) + 1);
}

ZTEST_SUITE(storage, NULL, NULL, NULL, NULL, NULL);

void TestLogCallback(const char* module, aos::LogLevel level, const aos::String& message)
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

    instanceInfo2.mPriority    = 3;
    instanceInfo2.mStoragePath = "storage_path";
    zassert_equal(storage.AddInstance(instanceInfo2), aos::ErrorEnum::eAlreadyExist, "Unexpected error");

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

    serviceData2.mImagePath  = "image_path3";
    serviceData2.mProviderID = "provider_id3";
    zassert_equal(storage.AddService(serviceData2), aos::ErrorEnum::eAlreadyExist, "Unexpected error");

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

ZTEST(storage, test_AddRemoveCertInfo)
{
    aos::Log::SetCallback(TestLogCallback);

    Storage storage;

    zassert_equal(storage.Init(), aos::ErrorEnum::eNone, "Failed to initialize storage");

    aos::StaticArray<aos::iam::certhandler::CertInfo, 2> certInfos;

    const aos::String certTypePkcs = "pkcs11";

    auto certTime = aos::Time::Now();

    aos::iam::certhandler::CertInfo certInfo;
    certInfo.mIssuer   = StringToDN("issuer");
    certInfo.mSerial   = StringToDN("serial");
    certInfo.mCertURL  = "cert_url";
    certInfo.mKeyURL   = "key_url";
    certInfo.mNotAfter = certTime;

    certInfos.PushBack(certInfo);

    aos::iam::certhandler::CertInfo certInfo2;
    certInfo2.mIssuer  = StringToDN("issuer2");
    certInfo2.mSerial  = StringToDN("serial2");
    certInfo2.mCertURL = "cert_url2";
    certInfo2.mKeyURL  = "key_url2";

    certInfos.PushBack(certInfo2);

    const aos::String certTypeTpm = "tpm";

    aos::iam::certhandler::CertInfo certInfoTpm;
    certInfoTpm.mIssuer  = StringToDN("issuer_tpm");
    certInfoTpm.mSerial  = StringToDN("serial_tpm");
    certInfoTpm.mCertURL = "cert_url_tpm";
    certInfoTpm.mKeyURL  = "key_url_tpm";

    for (const auto& certInfo : certInfos) {
        zassert_equal(storage.AddCertInfo(certTypePkcs, certInfo), aos::ErrorEnum::eNone, "Failed to add cert info");
    }

    aos::StaticArray<aos::iam::certhandler::CertInfo, 2> certInfos2;

    zassert_equal(
        storage.GetCertsInfo(certTypePkcs, certInfos2), aos::ErrorEnum::eNone, "Failed to get all cert infos");
    zassert_equal(certInfos2.Size(), 2, "Unexpected number of cert infos");

    for (const auto& certInfo : certInfos2) {
        zassert_equal(certInfos.Find(certInfo).mError, aos::ErrorEnum::eNone, "Unexpected cert info");
    }

    zassert_equal(storage.AddCertInfo(certTypeTpm, certInfoTpm), aos::ErrorEnum::eNone, "Failed to add cert info");

    certInfos2.Clear();

    zassert_equal(storage.GetCertsInfo(certTypeTpm, certInfos2), aos::ErrorEnum::eNone, "Failed to get all cert infos");
    zassert_equal(certInfos2.Size(), 1, "Unexpected number of cert infos");
    zassert_equal(certInfos2[0] == certInfoTpm, true, "certInfoTpm != certInfos3[0]");

    aos::iam::certhandler::CertInfo certInfoGet;

    zassert_equal(storage.GetCertInfo(certInfoTpm.mIssuer, certInfoTpm.mSerial, certInfoGet), aos::ErrorEnum::eNone,
        "Failed to get cert info");
    zassert_equal(certInfoGet == certInfoTpm, true, "certInfoTpm != certInfoGet");

    certInfoTpm.mCertURL = "cert_url_tpm2";
    certInfoTpm.mKeyURL  = "key_url_tpm2";
    zassert_equal(storage.AddCertInfo(certTypeTpm, certInfoTpm), aos::ErrorEnum::eAlreadyExist, "Unexpected error");

    zassert_equal(
        storage.RemoveCertInfo(certTypePkcs, certInfo.mCertURL), aos::ErrorEnum::eNone, "Failed to remove cert info");

    certInfos2.Clear();

    zassert_equal(
        storage.GetCertsInfo(certTypePkcs, certInfos2), aos::ErrorEnum::eNone, "Failed to get all cert infos");
    zassert_equal(certInfos2.Size(), 1, "Unexpected number of cert infos");
    zassert_equal(certInfos2[0] == certInfo2, true, "certInfo2 != certInfos2[0]");
    zassert_equal(storage.RemoveAllCertsInfo(certTypePkcs), aos::ErrorEnum::eNone, "Failed to remove all cert infos");

    certInfos2.Clear();

    zassert_equal(
        storage.GetCertsInfo(certTypePkcs, certInfos2), aos::ErrorEnum::eNone, "Failed to get all cert infos");
    zassert_equal(certInfos2.Size(), 0, "Unexpected number of cert infos");
    zassert_equal(storage.RemoveAllCertsInfo(certTypeTpm), aos::ErrorEnum::eNone, "Failed to remove all cert infos");

    certInfos2.Clear();

    zassert_equal(storage.GetCertsInfo(certTypeTpm, certInfos2), aos::ErrorEnum::eNone, "Failed to get all cert infos");
    zassert_equal(certInfos2.Size(), 0, "Unexpected number of cert infos");
}
