#pragma once

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <chrono>
#include <thread>

#include "queue.h"

class CSharedQueue
{
    constexpr static const auto PollInterval = std::chrono::microseconds(10);

public:
    CSharedQueue(size_t messageSize, bool createBuffer) // if !createBuffer will wait indefinitely for shmem to be created
        : m_sharedBufferSize(sizeof(CSharedQueueCore) + CSharedQueueCore::GetRequiredBufferSize(messageSize))
        , m_unlinkSharedBuffer(createBuffer)
        , m_pSharedBuffer(nullptr)
    {
        bool sharedBufferWasCreated = false;

        if (createBuffer)
        {
            m_sharedMemoryFD = shm_open(SharedMemoryName, O_RDWR, 0666);
            if (m_sharedMemoryFD == -1)
            {
                m_sharedMemoryFD = shm_open(SharedMemoryName, O_CREAT | O_RDWR, 0666);
                if (m_sharedMemoryFD != -1)
                {
                    ftruncate(m_sharedMemoryFD, m_sharedBufferSize);
                    sharedBufferWasCreated = true;
                }
            }
        }
        else
        {
            bool retry = false;
            do
            {
                m_sharedMemoryFD = shm_open(SharedMemoryName, O_RDWR, 0666);
                retry = (m_sharedMemoryFD == -1) && (errno == ENOENT);
                if (retry)
                {
                    std::this_thread::sleep_for(PollInterval);  // for now... [TODO] make better wait
                }
            }while(retry);
        }

        m_pSharedBuffer = static_cast<unsigned char *>(mmap(nullptr,
                                                            m_sharedBufferSize,
                                                            PROT_READ | PROT_WRITE,
                                                            MAP_SHARED,
                                                            m_sharedMemoryFD,
                                                            0
                                                            ));

        if (sharedBufferWasCreated)
        {
            new (m_pSharedBuffer) CSharedQueueCore(messageSize);
        }
    }

    ~CSharedQueue()
    {
        munmap(m_pSharedBuffer, m_sharedBufferSize);
        close(m_sharedMemoryFD);
        if (m_unlinkSharedBuffer)
        {
            unlinkBuffer();
        }
    }

    unsigned char *frontMessagePtr() const
    { return m_pSharedBuffer + QueueDataBufferOffset + queue().frontMessageOffset(); }

    unsigned char *nextMessagePtr ()
    { return m_pSharedBuffer + QueueDataBufferOffset + queue().nextMessageOffset(); }

    auto messageSize() const { return queue().messageSize(); }
    auto empty() const { return queue().empty(); }
    auto full() const { return queue().full(); }
    void pop_front() { queue().pop_front(); }
    void push_back() { queue().push_back(); }

    void unlinkBuffer() const { shm_unlink(SharedMemoryName); }

private:
    constexpr static const auto SharedMemoryName = "IPCTestQueueSharedBuffer";
    static const auto QueueDataBufferOffset = sizeof(CSharedQueueCore);


    const CSharedQueueCore &queue() const
    {
        assert(m_pSharedBuffer != nullptr);
        const auto pQueue = reinterpret_cast<const CSharedQueueCore*>(m_pSharedBuffer);
        return *pQueue;
    }

    CSharedQueueCore &queue()
    {
        assert(m_pSharedBuffer != nullptr);
        const auto pQueue = reinterpret_cast<CSharedQueueCore*>(m_pSharedBuffer);
        return *pQueue;
    }


    const size_t m_sharedBufferSize;
    const bool m_unlinkSharedBuffer;
    int m_sharedMemoryFD;
    unsigned char *m_pSharedBuffer;
};
