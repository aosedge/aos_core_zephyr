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
#include "utils/log.hpp"
#include "utils/utils.hpp"

namespace aos::zephyr {

/***********************************************************************************************************************
 * Types
 **********************************************************************************************************************/

struct storage_fixture {
    storage::Storage mStorage;
};

/***********************************************************************************************************************
 * Static
 **********************************************************************************************************************/

namespace {

constexpr auto cStoragePath = CONFIG_AOS_STORAGE_DIR;

const aos::Array<uint8_t> StringToDN(const char* str)
{
    return aos::Array<uint8_t>(reinterpret_cast<const uint8_t*>(str), strlen(str) + 1);
}

} // namespace

/***********************************************************************************************************************
 * Setup
 **********************************************************************************************************************/

ZTEST_SUITE(
    storage, NULL,
    []() -> void* {
        aos::Log::SetCallback(TestLogCallback);

        assert(!String(cStoragePath).IsEmpty());

        auto err = FS::ClearDir(cStoragePath);
        zassert_true(err.IsNone(), "Failed to clear storage directory: err= %s", utils::ErrorToCStr(err));

        auto fixture = new storage_fixture;

        err = fixture->mStorage.Init();
        zassert_true(err.IsNone(), "Can't initialize IAM client: %s", utils::ErrorToCStr(err));

        return fixture;
    },
    NULL, NULL, NULL);

/***********************************************************************************************************************
 * Tests
 **********************************************************************************************************************/

ZTEST_F(storage, test_AddUpdateRemoveInstance)
{
    storage::Storage& storage = fixture->mStorage;

    aos::StaticArray<sm::launcher::InstanceData, 2> instances;

    sm::launcher::InstanceData instanceInfo;
    instanceInfo.mInstanceID                             = "id1";
    instanceInfo.mInstanceInfo.mInstanceIdent.mInstance  = 1;
    instanceInfo.mInstanceInfo.mInstanceIdent.mServiceID = "service_id";
    instanceInfo.mInstanceInfo.mInstanceIdent.mSubjectID = "subject_id";
    instanceInfo.mInstanceInfo.mPriority                 = 1;
    instanceInfo.mInstanceInfo.mStoragePath              = "storage_path";
    instanceInfo.mInstanceInfo.mStatePath                = "state_path";
    instanceInfo.mInstanceInfo.mUID                      = 1;

    instances.PushBack(instanceInfo);

    sm::launcher::InstanceData instanceInfo2;
    instanceInfo2.mInstanceID                             = "id2";
    instanceInfo2.mInstanceInfo.mInstanceIdent.mInstance  = 2;
    instanceInfo2.mInstanceInfo.mInstanceIdent.mServiceID = "service_id";
    instanceInfo2.mInstanceInfo.mInstanceIdent.mSubjectID = "subject_id";
    instanceInfo2.mInstanceInfo.mPriority                 = 2;
    instanceInfo2.mInstanceInfo.mStoragePath              = "storage_path";
    instanceInfo2.mInstanceInfo.mStatePath                = "state_path";
    instanceInfo2.mInstanceInfo.mUID                      = 2;

    instances.PushBack(instanceInfo2);

    for (const auto& instance : instances) {
        zassert_equal(storage.AddInstance(instance), aos::ErrorEnum::eNone, "Failed to add instance");
    }

    for (const auto& instance : instances) {
        zassert_equal(storage.AddInstance(instance), aos::ErrorEnum::eAlreadyExist, "Unexpected error");
    }

    instanceInfo2.mInstanceInfo.mPriority    = 3;
    instanceInfo2.mInstanceInfo.mStoragePath = "storage_path";
    zassert_equal(storage.AddInstance(instanceInfo2), aos::ErrorEnum::eAlreadyExist, "Unexpected error");

    aos::StaticArray<sm::launcher::InstanceData, 2> instances2;
    zassert_equal(storage.GetAllInstances(instances2), aos::ErrorEnum::eNone, "Failed to get all instances");

    zassert_equal(instances2.Size(), 2, "Unexpected number of instances");

    for (const auto& instance : instances2) {
        zassert_not_equal(instances.Find(instance), instances.end(), "Unexpected instance");
    }

    instances[0].mInstanceInfo.mStoragePath = "storage_path2";
    instances[0].mInstanceInfo.mStatePath   = "state_path2";

    instances[1].mInstanceInfo.mStoragePath = "storage_path3";
    instances[1].mInstanceInfo.mStatePath   = "state_path3";

    for (const auto& instance : instances) {
        zassert_equal(storage.UpdateInstance(instance), aos::ErrorEnum::eNone, "Failed to update instance");
    }

    instances2.Clear();
    zassert_equal(storage.GetAllInstances(instances2), aos::ErrorEnum::eNone, "Failed to get all instances");

    zassert_equal(instances2.Size(), 2, "Unexpected number of instances");

    for (const auto& instance : instances2) {
        zassert_not_equal(instances.Find(instance), instances.end(), "Unexpected instance");
    }

    for (const auto& instance : instances) {
        zassert_equal(storage.RemoveInstance(instance.mInstanceID), aos::ErrorEnum::eNone, "Failed to remove instance");
    }

    instances2.Clear();
    zassert_equal(storage.GetAllInstances(instances2), aos::ErrorEnum::eNone, "Failed to get all instances");

    zassert_equal(instances2.Size(), 0, "Unexpected number of instances");
}

ZTEST_F(storage, test_AddUpdateRemoveService)
{
    storage::Storage& storage = fixture->mStorage;

    aos::StaticArray<aos::sm::servicemanager::ServiceData, 2> services;

    aos::sm::servicemanager::ServiceData serviceData {};

    serviceData.mServiceID  = "service_id";
    serviceData.mProviderID = "provider_id";
    serviceData.mVersion    = "1.0.0";
    serviceData.mImagePath  = "image_path";

    services.PushBack(serviceData);

    aos::sm::servicemanager::ServiceData serviceData2 {};

    serviceData2.mServiceID  = "service_id2";
    serviceData2.mProviderID = "provider_id2";
    serviceData2.mVersion    = "2.0.0";
    serviceData2.mImagePath  = "image_path2";

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
        zassert_not_equal(services.Find(service), services.end(), "Unexpected service");
    }

    {
        StaticArray<sm::servicemanager::ServiceData, 2> serviceVersions;

        zassert_equal(storage.GetServiceVersions(serviceData.mServiceID, serviceVersions), aos::ErrorEnum::eNone,
            "Failed to get service");

        LOG_MODULE_DBG("storage") << "serviceID: " << serviceData.mVersion.CStr()
                                  << ", count=" << serviceVersions.Size();

        zassert_equal(serviceVersions.Size(), 1, "Unexpected service versions");

        printk("serviceID: %s\n", serviceData.mVersion.CStr());
        printk("serviceID: %s\n", serviceVersions[0].mVersion.CStr());

        zassert_equal(serviceVersions[0], serviceData, "Unexpected service");
    }

    services[0].mImagePath = "image_path3";
    services[1].mImagePath = "image_path4";

    for (const auto& service : services) {
        zassert_equal(storage.UpdateService(service), aos::ErrorEnum::eNone, "Failed to update service");
    }

    services2.Clear();
    zassert_equal(storage.GetAllServices(services2), aos::ErrorEnum::eNone, "Failed to get all services");

    zassert_equal(services2.Size(), 2, "Unexpected number of services");

    for (const auto& service : services2) {
        zassert_not_equal(services.Find(service), services.end(), "Unexpected service");
    }

    for (const auto& service : services) {
        zassert_equal(storage.RemoveService(service.mServiceID, service.mVersion), aos::ErrorEnum::eNone,
            "Failed to remove service");
    }

    services2.Clear();
    zassert_equal(storage.GetAllServices(services2), aos::ErrorEnum::eNone, "Failed to get all services");

    zassert_equal(services2.Size(), 0, "Unexpected number of services");
}

ZTEST_F(storage, test_AddUpdateRemoveLayer)
{
    storage::Storage& storage = fixture->mStorage;

    StaticArray<sm::layermanager::LayerData, 2> layers;

    sm::layermanager::LayerData layerData {};

    layerData.mLayerDigest         = "layer-digest";
    layerData.mUnpackedLayerDigest = "unpacked-layer-digest";
    layerData.mLayerID             = "layer-id";
    layerData.mVersion             = "1.0.0";
    layerData.mPath                = "layer-path";
    layerData.mOSVersion           = "layer-os-version";
    layerData.mTimestamp           = Time::Now();
    layerData.mState               = sm::layermanager::LayerStateEnum::eActive;
    layerData.mSize                = 128;

    layers.PushBack(layerData);

    sm::layermanager::LayerData layerData2 {};

    layerData2.mLayerDigest         = "layer-digest-2";
    layerData2.mUnpackedLayerDigest = "unpacked-layer-digest-2";
    layerData2.mLayerID             = "layer-id";
    layerData2.mVersion             = "1.0.0";
    layerData2.mPath                = "layer-path";
    layerData2.mOSVersion           = "layer-os-version";
    layerData2.mTimestamp           = Time::Now();
    layerData2.mState               = sm::layermanager::LayerStateEnum::eCached;
    layerData2.mSize                = 256;

    layers.PushBack(layerData2);

    for (const auto& layer : layers) {
        zassert_equal(storage.AddLayer(layer), ErrorEnum::eNone, "Failed to add layer");
    }

    for (const auto& layer : layers) {
        zassert_equal(storage.AddLayer(layer), ErrorEnum::eAlreadyExist, "Unexpected error");
    }

    layerData2.mTimestamp = Time::Now();
    layerData2.mSize      = 512;
    zassert_equal(storage.AddLayer(layerData2), ErrorEnum::eAlreadyExist, "Unexpected error");

    StaticArray<sm::layermanager::LayerData, 2> layers2;

    zassert_equal(storage.GetAllLayers(layers2), ErrorEnum::eNone, "Failed to get all layers");

    zassert_equal(layers2.Size(), 2, "Unexpected number of layers");

    for (const auto& layer : layers2) {
        zassert_not_equal(layers.Find(layer), layers.end(), "Unexpected layers");
    }

    layers[0].mTimestamp = Time::Now();
    layers[1].mTimestamp = Time::Now();

    for (const auto& layer : layers) {
        zassert_equal(storage.UpdateLayer(layer), ErrorEnum::eNone, "Failed to update layer");
    }

    layers2.Clear();
    zassert_equal(storage.GetAllLayers(layers2), ErrorEnum::eNone, "Failed to get all layers");

    zassert_equal(layers2.Size(), 2, "Unexpected number of layers");

    for (const auto& layer : layers2) {
        zassert_not_equal(layers.Find(layer), layers.end(), "Unexpected layer");
    }

    for (const auto& layer : layers) {
        zassert_equal(storage.RemoveLayer(layer.mLayerDigest), ErrorEnum::eNone, "Failed to remove layer");
    }

    layers2.Clear();
    zassert_equal(storage.GetAllLayers(layers2), ErrorEnum::eNone, "Failed to get all layers");

    zassert_equal(layers2.Size(), 0, "Unexpected number of layers");
}

ZTEST_F(storage, test_AddRemoveCertInfo)
{
    storage::Storage& storage = fixture->mStorage;

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
        zassert_not_equal(certInfos.Find(certInfo), certInfos.end(), "Unexpected cert info");
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

} // namespace aos::zephyr
