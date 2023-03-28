/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FILE_STORAGE_HPP_
#define FILE_STORAGE_HPP_

#include <fcntl.h>
#include <sys/stat.h>
#include <tinycrypt/constants.h>
#include <tinycrypt/sha256.h>
#include <unistd.h>

#include "aos/common/tools/error.hpp"

#include "aos/common/tools/noncopyable.hpp"
#include "aos/common/tools/string.hpp"
#include "aos/common/types.hpp"

#include "log.hpp"

template <typename T>
class FileStorage : public aos::NonCopyable {
public:
    /**
     * Default constructor.
     */
    FileStorage() = default;

    /**
     * Destructor.
     */
    ~FileStorage()
    {
        if (mFd >= 0) {
            close(mFd);
        }
    }

    /**
     * Initialize database.
     *
     * @param path Path to database file.
     * @return Error.
     */
    aos::Error Init(const aos::String& path)
    {
        mFd = open(path.CStr(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
        if (mFd < 0) {
            return AOS_ERROR_WRAP(mFd);
        }

        off_t fileSize = lseek(mFd, 0, SEEK_END);
        if (fileSize == -1) {
            return aos::ErrorEnum::eRuntime;
        }

        if (fileSize == 0) {
            Header header;

            auto err = CalculateSha256(header, sizeof(Header), header.mChecksum);
            if (!err.IsNone()) {
                return err;
            }

            ssize_t nwrite = write(mFd, &header, sizeof(Header));
            if (nwrite != sizeof(Header)) {
                return aos::ErrorEnum::eRuntime;
            }
        }

        return aos::ErrorEnum::eNone;
    }

    /**
     * Add new record to database.
     *
     * @param data Record to add.
     * @return Error
     */
    aos::Error Add(const T& data)
    {
        auto ret = lseek(mFd, sizeof(Header), SEEK_SET);
        if (ret < 0) {
            return AOS_ERROR_WRAP(ret);
        }

        Record  record {};
        ssize_t nread {};

        while ((nread = read(mFd, &record, sizeof(Record))) == sizeof(Record)) {
            if (record.mDeleted) {
                off_t offset = -static_cast<off_t>(sizeof(Record));

                auto ret = lseek(mFd, offset, SEEK_CUR);
                if (ret < 0) {
                    return AOS_ERROR_WRAP(ret);
                }

                record.mData = data;
                record.mDeleted = 0;

                auto err = CalculateSha256(data, sizeof(T), record.mChecksum);
                if (!err.IsNone()) {
                    return err;
                }

                ssize_t nwrite = write(mFd, &record, sizeof(Record));
                if (nwrite != sizeof(Record)) {
                    return aos::ErrorEnum::eRuntime;
                }

                return aos::ErrorEnum::eNone;
            }
        }

        if (nread < 0) {
            return AOS_ERROR_WRAP(nread);
        }

        record.mData = data;
        record.mDeleted = 0;

        auto err = CalculateSha256(data, sizeof(T), record.mChecksum);
        if (!err.IsNone()) {
            return err;
        }

        ssize_t nwrite = write(mFd, &record, sizeof(Record));
        if (nwrite != sizeof(Record)) {
            return aos::ErrorEnum::eRuntime;
        }

        return aos::ErrorEnum::eNone;
    }

    /**
     * Update record in database.
     *
     * @tparam F Filter type.
     * @param data Record to update.
     * @param filter Filter to find record.
     * @return Error
     */
    template <typename F>
    aos::Error Update(const T& data, F filter)
    {
        auto ret = lseek(mFd, sizeof(Header), SEEK_SET);
        if (ret < 0) {
            return AOS_ERROR_WRAP(ret);
        }

        Record  record {};
        ssize_t nread {};

        while ((nread = read(mFd, &record, sizeof(Record))) == sizeof(Record)) {
            if (record.mDeleted) {
                continue;
            }

            if (filter(record.mData)) {
                off_t offset = -static_cast<off_t>(sizeof(Record));

                ret = lseek(mFd, offset, SEEK_CUR);
                if (ret < 0) {
                    return AOS_ERROR_WRAP(ret);
                }

                record.mData = data;

                auto err = CalculateSha256(data, sizeof(T), record.mChecksum);
                if (!err.IsNone()) {
                    return err;
                }

                ssize_t nwrite = write(mFd, &record, sizeof(Record));
                if (nwrite != sizeof(Record)) {
                    return aos::ErrorEnum::eRuntime;
                }

                return aos::ErrorEnum::eNone;
            }
        }

        if (nread < 0) {
            return AOS_ERROR_WRAP(nread);
        }

        return aos::ErrorEnum::eNotFound;
    }

    /**
     * Remove record from database.
     *
     * @tparam F Filter type.
     * @param filter Filter to find record.
     * @return Error
     */
    template <typename F>
    aos::Error Remove(F filter)
    {
        auto ret = lseek(mFd, sizeof(Header), SEEK_SET);
        if (ret < 0) {
            return AOS_ERROR_WRAP(ret);
        }

        Record  record {};
        ssize_t nread {};

        while ((nread = read(mFd, &record, sizeof(Record))) == sizeof(Record)) {
            if (record.mDeleted) {
                continue;
            }

            if (filter(record.mData)) {
                off_t offset = -static_cast<off_t>(sizeof(Record));

                ret = lseek(mFd, offset, SEEK_CUR);
                if (ret < 0) {
                    return AOS_ERROR_WRAP(ret);
                }

                record.mDeleted = 1;

                ssize_t nwrite = write(mFd, &record, sizeof(Record));
                if (nwrite != sizeof(Record)) {
                    return aos::ErrorEnum::eRuntime;
                }

                return aos::ErrorEnum::eNone;
            }
        }

        if (nread < 0) {
            return AOS_ERROR_WRAP(nread);
        }

        return aos::ErrorEnum::eNotFound;
    }

    /**
     * Read all records from database.
     *
     * @tparam F Filter type.
     * @param append Append function.
     * @return Error
     */
    template <typename F>
    aos::Error ReadRecords(F append)
    {
        auto ret = lseek(mFd, sizeof(Header), SEEK_SET);
        if (ret < 0) {
            return AOS_ERROR_WRAP(ret);
        }

        Record  record {};
        ssize_t nread {};

        while ((nread = read(mFd, &record, sizeof(Record))) == sizeof(Record)) {
            if (record.mDeleted) {
                continue;
            }

            auto err = append(record.mData);
            if (!err.IsNone()) {
                return err;
            }
        }

        if (nread < 0) {
            return AOS_ERROR_WRAP(nread);
        }

        return aos::ErrorEnum::eNone;
    }

    /**
     * Read record from database by filter.
     *
     * @tparam F Filter type.
     * @param data Data to read.
     * @param filter Filter to find record.
     * @return Error
     */
    template <typename F>
    aos::Error ReadRecordByFilter(T& data, F filter)
    {
        auto ret = lseek(mFd, sizeof(Header), SEEK_SET);
        if (ret < 0) {
            return AOS_ERROR_WRAP(ret);
        }

        Record  record {};
        ssize_t nread {};

        while ((nread = read(mFd, &record, sizeof(Record))) == sizeof(Record)) {
            if (record.mDeleted) {
                continue;
            }

            if (filter(record.mData)) {
                data = record.mData;

                return aos::ErrorEnum::eNone;
            }
        }

        if (nread < 0) {
            return AOS_ERROR_WRAP(nread);
        }

        return aos::ErrorEnum::eNotFound;
    }

private:
    struct Header {
        uint64_t                                    mVersion {};
        uint8_t                                     mReserved[256] {};
        aos::StaticArray<uint8_t, aos::cSHA256Size> mChecksum;
    };

    struct Record {
        T                                           mData;
        uint8_t                                     mDeleted;
        aos::StaticArray<uint8_t, aos::cSHA256Size> mChecksum;
    };

    template <typename T1>
    aos::Error CalculateSha256(const T1& data, size_t msgSize, aos::StaticArray<uint8_t, aos::cSHA256Size>& digest)
    {
        tc_sha256_state_struct s;

        auto ret = tc_sha256_init(&s);
        if (TC_CRYPTO_SUCCESS != ret) {
            return AOS_ERROR_WRAP(ret);
        }

        ret = tc_sha256_update(&s, (const uint8_t*)(&data), msgSize);
        if (TC_CRYPTO_SUCCESS != ret) {
            return AOS_ERROR_WRAP(ret);
        }

        ret = tc_sha256_final(digest.Get(), &s);
        if (TC_CRYPTO_SUCCESS != ret) {
            return AOS_ERROR_WRAP(ret);
        }

        return aos::ErrorEnum::eNone;
    }

    int mFd = -1;
};

// #include "filestorage.tpp"

#endif
