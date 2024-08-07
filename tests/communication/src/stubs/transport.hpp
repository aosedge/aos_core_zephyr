#include <atomic>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <vector>

#include "communication/transport.hpp"

namespace aos::zephyr::communication {

class Pipe {
public:
    Pipe()
        : mClosed(false)
    {
    }

    void Write(const uint8_t* data, size_t size)
    {
        std::unique_lock<std::mutex> lock(mMutex);
        mBuffer.insert(mBuffer.end(), data, data + size);
        mCondVar.notify_one();
    }

    size_t Read(uint8_t* data, size_t size)
    {
        std::unique_lock<std::mutex> lock(mMutex);

        mCondVar.wait(lock, [this] { return !mBuffer.empty() || mClosed; });
        if (mClosed || mBuffer.empty()) {
            return -1;
        }

        size_t bytesRead = std::min(size, mBuffer.size());
        std::copy(mBuffer.begin(), mBuffer.begin() + bytesRead, data);
        mBuffer.erase(mBuffer.begin(), mBuffer.begin() + bytesRead);

        std::cout << "Read " << bytesRead << " bytes" << std::endl;

        return bytesRead;
    }

    void Close()
    {
        std::unique_lock<std::mutex> lock(mMutex);
        mClosed = true;
        mCondVar.notify_all();
    }

private:
    std::vector<uint8_t>    mBuffer;
    std::mutex              mMutex;
    std::condition_variable mCondVar;
    std::atomic<bool>       mClosed;
};

class TransportStub : public TransportItf {
public:
    TransportStub(Pipe& readPipe, Pipe& writePipe)
        : mReadPipe(readPipe)
        , mWritePipe(writePipe)
        , mIsOpened(true)
    {
    }

    Error Open() override
    {
        mIsOpened.store(true);

        return ErrorEnum::eNone;
    }

    Error Close() override
    {
        mIsOpened.store(false);
        return ErrorEnum::eNone;
    }

    bool IsOpened() const override { return mIsOpened.load(); }

    int Read(void* data, size_t size) override
    {
        if (!mIsOpened.load()) {
            return -1;
        }

        return static_cast<int>(mReadPipe.Read(static_cast<uint8_t*>(data), size));
    }

    int Write(const void* data, size_t size) override
    {
        if (!mIsOpened.load()) {
            return -1;
        }

        mWritePipe.Write(static_cast<const uint8_t*>(data), size);

        return static_cast<int>(size);
    }

private:
    Pipe&             mReadPipe;
    Pipe&             mWritePipe;
    std::atomic<bool> mIsOpened;
};

} // namespace aos::zephyr::communication
