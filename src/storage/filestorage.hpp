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

#include <unistd.h>

#include <aos/common/tools/error.hpp>
#include <aos/common/tools/noncopyable.hpp>
#include <aos/common/tools/string.hpp>

#include "utils/checksum.hpp"

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
     * Initializes the database.
     *
     * @param path Path to database file.
     * @return Error.
     */
    aos::Error Init(const aos::String& path)
    {
        mFileName = path;

        mFd = open(path.CStr(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
        if (mFd < 0) {
            return AOS_ERROR_WRAP(errno);
        }

        off_t fileSize = lseek(mFd, 0, SEEK_END);
        if (fileSize == -1) {
            return AOS_ERROR_WRAP(errno);
        }

        if (fileSize == 0) {
            Header header {};

            auto checksum = CalculateSha256(
                aos::Array<uint8_t>(reinterpret_cast<uint8_t*>(&header), sizeof(Header) - aos::cSHA256Size));
            if (!checksum.mError.IsNone()) {
                return AOS_ERROR_WRAP(checksum.mError);
            }

            ssize_t nwrite = write(mFd, &header, sizeof(Header));
            if (nwrite != sizeof(Header)) {
                return nwrite < 0 ? AOS_ERROR_WRAP(errno) : aos::ErrorEnum::eRuntime;
            }
        }

        auto err = Sync();
        if (!err.IsNone()) {
            return err;
        }

        return aos::ErrorEnum::eNone;
    }

    /**
     * Adds a new record to the database.
     *
     * @param data Record to add.
     * @return Error
     */
    aos::Error Add(const T& data)
    {
        auto ret = lseek(mFd, sizeof(Header), SEEK_SET);
        if (ret < 0) {
            return AOS_ERROR_WRAP(errno);
        }

        Record  record {};
        ssize_t nread {};
        off_t   deletedRecordOffset {-1};

        while ((nread = read(mFd, &record, sizeof(Record))) == sizeof(Record)) {
            if (!record.mDeleted && record.mData == data) {
                return aos::ErrorEnum::eAlreadyExist;
            }

            if (deletedRecordOffset == -1 && record.mDeleted) {
                deletedRecordOffset = lseek(mFd, 0, SEEK_CUR) - static_cast<off_t>(sizeof(Record));
                if (deletedRecordOffset < 0) {
                    return AOS_ERROR_WRAP(errno);
                }
            }
        }

        if (nread < 0) {
            return AOS_ERROR_WRAP(errno);
        }

        record.mData    = data;
        record.mDeleted = 0;

        auto checksum = CalculateSha256(
            aos::Array<uint8_t>(reinterpret_cast<uint8_t*>(&record), sizeof(record) - aos::cSHA256Size));
        if (!checksum.mError.IsNone()) {
            return checksum.mError;
        }

        aos::Array<uint8_t>(reinterpret_cast<uint8_t*>(record.mChecksum), aos::cSHA256Size) = checksum.mValue;

        if (deletedRecordOffset >= 0) {
            ret = lseek(mFd, deletedRecordOffset, SEEK_SET);
            if (ret < 0) {
                return AOS_ERROR_WRAP(errno);
            }
        }

        ssize_t nwrite = write(mFd, &record, sizeof(Record));
        if (nwrite != sizeof(Record)) {
            return nwrite < 0 ? AOS_ERROR_WRAP(errno) : aos::ErrorEnum::eRuntime;
        }

        auto err = Sync();
        if (!err.IsNone()) {
            return err;
        }

        return aos::ErrorEnum::eNone;
    }

    /**
     * Updates the records in the database.
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
            return AOS_ERROR_WRAP(errno);
        }

        Record  record {};
        ssize_t nread {};

        while ((nread = read(mFd, &record, sizeof(Record))) == sizeof(Record)) {
            if (record.mDeleted) {
                continue;
            }

            if (!filter(record.mData)) {
                continue;
            }

            off_t offset = -static_cast<off_t>(sizeof(Record));

            ret = lseek(mFd, offset, SEEK_CUR);
            if (ret < 0) {
                return AOS_ERROR_WRAP(errno);
            }

            record.mData = data;

            auto checksum = CalculateSha256(
                aos::Array<uint8_t>(reinterpret_cast<uint8_t*>(&record), sizeof(record) - aos::cSHA256Size));
            if (!checksum.mError.IsNone()) {
                return checksum.mError;
            }

            aos::Array<uint8_t>(reinterpret_cast<uint8_t*>(record.mChecksum), aos::cSHA256Size) = checksum.mValue;

            ssize_t nwrite = write(mFd, &record, sizeof(Record));
            if (nwrite != sizeof(Record)) {
                return nwrite < 0 ? AOS_ERROR_WRAP(errno) : aos::ErrorEnum::eRuntime;
            }

            return aos::ErrorEnum::eNone;
        }

        if (nread < 0) {
            return AOS_ERROR_WRAP(errno);
        }

        auto err = Sync();
        if (!err.IsNone()) {
            return err;
        }

        return aos::ErrorEnum::eNotFound;
    }

    /**
     * Removes record from the database.
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
            return AOS_ERROR_WRAP(errno);
        }

        Record  record {};
        ssize_t nread {};

        while ((nread = read(mFd, &record, sizeof(Record))) == sizeof(Record)) {
            if (record.mDeleted) {
                continue;
            }

            if (!filter(record.mData)) {
                continue;
            }

            off_t offset = -static_cast<off_t>(sizeof(Record));

            ret = lseek(mFd, offset, SEEK_CUR);
            if (ret < 0) {
                return AOS_ERROR_WRAP(errno);
            }

            record.mDeleted = 1;

            ssize_t nwrite = write(mFd, &record, sizeof(Record));
            if (nwrite != sizeof(Record)) {
                return nwrite < 0 ? AOS_ERROR_WRAP(errno) : aos::ErrorEnum::eRuntime;
            }

            auto err = Sync();
            if (!err.IsNone()) {
                return err;
            }

            return aos::ErrorEnum::eNone;
        }

        if (nread < 0) {
            return AOS_ERROR_WRAP(errno);
        }

        return aos::ErrorEnum::eNotFound;
    }

    /**
     * Reads all records from the database.
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
            return AOS_ERROR_WRAP(errno);
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
            return AOS_ERROR_WRAP(errno);
        }

        return aos::ErrorEnum::eNone;
    }

    /**
     * Reads record from the database by filter.
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
            return AOS_ERROR_WRAP(errno);
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
            return AOS_ERROR_WRAP(errno);
        }

        return aos::ErrorEnum::eNotFound;
    }

private:
    // Currently, zephyr doesn't provide Posix API (fsync) to flush the file correctly. We have to
    // rewrite this class using zephyr FS API. As workaround, we just reopen the file.
    aos::Error Sync()
    {
        if (mFd >= 0) {
            auto ret = close(mFd);
            if (ret < 0) {
                return AOS_ERROR_WRAP(errno);
            }
        }

        mFd = open(mFileName.CStr(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
        if (mFd < 0) {
            return AOS_ERROR_WRAP(errno);
        }

        return aos::ErrorEnum::eNone;
    }

    struct Header {
        uint64_t mVersion;
        uint8_t  mReserved[256];
        uint8_t  mChecksum[aos::cSHA256Size];
    };

    struct Record {
        T       mData;
        uint8_t mDeleted;
        uint8_t mChecksum[aos::cSHA256Size];
    };

    aos::StaticString<aos::cFilePathLen> mFileName;
    int                                  mFd {-1};
};

#endif
