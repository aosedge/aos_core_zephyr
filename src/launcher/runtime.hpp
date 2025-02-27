/*
 * Copyright (C) 2025 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_RUNTIME_HPP_
#define ZEPHYR_RUNTIME_HPP_

#include <aos/sm/launcher.hpp>

namespace aos::zephyr::launcher {

class Runtime : public aos::sm::launcher::RuntimeItf {
public:
    /**
     * Creates host FS whiteouts.
     *
     * @param path path to whiteouts.
     * @param hostBinds host binds.
     * @return Error.
     */
    Error CreateHostFSWhiteouts(const String& path, const Array<StaticString<cFilePathLen>>& hostBinds) override
    {
        (void)path;
        (void)hostBinds;

        return ErrorEnum::eNone;
    }

    /**
     * Creates mount points.
     *
     * @param mountPointDir mount point directory.
     * @param mounts mounts to create.
     * @return Error.
     */
    Error CreateMountPoints(const String& mountPointDir, const Array<Mount>& mounts) override
    {
        (void)mountPointDir;
        (void)mounts;

        return ErrorEnum::eNone;
    }

    /**
     * Mounts root FS for Aos service.
     *
     * @param rootfsPath path to service root FS.
     * @param layers layers to mount.
     * @return Error.
     */
    Error MountServiceRootFS(const String& rootfsPath, const Array<StaticString<cFilePathLen>>& layers) override
    {
        (void)rootfsPath;
        (void)layers;

        return ErrorEnum::eNone;
    }

    /**
     * Umounts Aos service root FS.
     *
     * @param rootfsPath path to service root FS.
     * @return Error.
     */
    Error UmountServiceRootFS(const String& rootfsPath) override
    {
        (void)rootfsPath;

        return ErrorEnum::eNone;
    }

    /**
     * Prepares Aos service storage directory.
     *
     * @param path service storage directory.
     * @param uid user ID.
     * @param gid group ID.
     * @return Error.
     */
    Error PrepareServiceStorage(const String& path, uint32_t uid, uint32_t gid) override
    {
        (void)path;
        (void)uid;
        (void)gid;

        return ErrorEnum::eNone;
    }

    /**
     * Prepares Aos service state file.
     *
     * @param path service state file path.
     * @param uid user ID.
     * @param gid group ID.
     * @return Error.
     */
    Error PrepareServiceState(const String& path, uint32_t uid, uint32_t gid) override
    {
        (void)path;
        (void)uid;
        (void)gid;

        return ErrorEnum::eNone;
    }

    /**
     * Prepares directory for network files.
     *
     * @param path network directory path.
     * @return Error.
     */
    Error PrepareNetworkDir(const String& path) override
    {
        (void)path;

        return ErrorEnum::eNone;
    }

    /**
     * Returns absolute path of FS item.
     *
     * @param path path to convert.
     * @return RetWithError<StaticString<cFilePathLen>>.
     */
    RetWithError<StaticString<cFilePathLen>> GetAbsPath(const String& path) override
    {
        return {path, ErrorEnum::eNone};
    }

    /**
     * Returns GID by group name.
     *
     * @param groupName group name.
     * @return RetWithError<uint32_t>.
     */
    RetWithError<uint32_t> GetGIDByName(const String& groupName) override
    {
        (void)groupName;

        return {0, ErrorEnum::eNone};
    }

    /**
     * Populates host devices.
     *
     * @param devicePath device path.
     * @param[out] devices OCI devices.
     * @return Error.
     */
    Error PopulateHostDevices(const String& devicePath, Array<oci::LinuxDevice>& devices) override
    {
        (void)devicePath;
        (void)devices;

        return ErrorEnum::eNone;
    }
};

} // namespace aos::zephyr::launcher

#endif
