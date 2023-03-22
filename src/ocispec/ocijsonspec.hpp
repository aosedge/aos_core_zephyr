/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef OCIJSONSPEC_HPP_
#define OCIJSONSPEC_HPP_

#include <zephyr/data/json.h>

#include <aos/common/osicpec.hpp>

static constexpr size_t cMaxDeviceTreeLength = 1024;

struct HypervisorSpec {
    char*   path;
    char*   parameters[aos::oci::cMaxParamCount];
    uint8_t paramsLen;
};

struct KernelSpec {
    char*   path;
    char*   parameters[aos::oci::cMaxParamCount];
    uint8_t paramsLen;
};

struct HWConfigSpec {
    char *devicetree;
};

struct VMSpec {
    HypervisorSpec hypervisor;
    KernelSpec     kernel;
    HWConfigSpec   hwconfig;
};

struct RuntimeSpec {
    char*  ociVersion;
    VMSpec vm;
};

static const struct json_obj_descr HypervisorSpecJsonDescr[] = {
    JSON_OBJ_DESCR_PRIM(struct HypervisorSpec, path, JSON_TOK_STRING),
    JSON_OBJ_DESCR_ARRAY(struct HypervisorSpec, parameters, aos::oci::cMaxParamCount, paramsLen, JSON_TOK_STRING),
};

static const struct json_obj_descr KernelSpecJsonDescr[] = {
    JSON_OBJ_DESCR_PRIM(struct KernelSpec, path, JSON_TOK_STRING),
    JSON_OBJ_DESCR_ARRAY(struct KernelSpec, parameters, aos::oci::cMaxParamCount, paramsLen, JSON_TOK_STRING),
};

static const struct json_obj_descr HWConfigSpecJsonDescr[] = {
    JSON_OBJ_DESCR_PRIM(struct HWConfigSpec, devicetree, JSON_TOK_STRING),
};

static const struct json_obj_descr VMSpecJsonDescr[] = {
    JSON_OBJ_DESCR_OBJECT(struct VMSpec, hypervisor, HypervisorSpecJsonDescr),
    JSON_OBJ_DESCR_OBJECT(struct VMSpec, kernel, KernelSpecJsonDescr),
    JSON_OBJ_DESCR_OBJECT(struct VMSpec, hwconfig, HWConfigSpecJsonDescr),
};

static const struct json_obj_descr RuntimeSpecJsonDescr[] = {
    JSON_OBJ_DESCR_PRIM(struct RuntimeSpec, ociVersion, JSON_TOK_STRING),
    JSON_OBJ_DESCR_OBJECT(struct RuntimeSpec, vm, VMSpecJsonDescr),
};

struct ImageConfig {
    char*  cmd[aos::oci::cMaxParamCount];
    char*  entrypoint[aos::oci::cMaxParamCount];
    size_t cmdLen;
    size_t entrypointLen;
};

struct ImageSpec {
    ImageConfig config;
};

const struct json_obj_descr ImageConfigJsonDescr[] = {
    JSON_OBJ_DESCR_ARRAY(struct ImageConfig, cmd, aos::oci::cMaxParamCount, cmdLen, JSON_TOK_STRING),
    JSON_OBJ_DESCR_ARRAY(struct ImageConfig, entrypoint, aos::oci::cMaxParamCount, entrypointLen, JSON_TOK_STRING),
};

const struct json_obj_descr ImageSpecJsonDescr[] = {
    JSON_OBJ_DESCR_OBJECT(struct ImageSpec, config, ImageConfigJsonDescr),
};

#endif
