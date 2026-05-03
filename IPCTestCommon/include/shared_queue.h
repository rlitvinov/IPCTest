#pragma once

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <chrono>
#include <thread>

#include "queue.h"
#include "signal_handlers.h"

class CSharedQueue
{
public:
    CSharedQueue(std::string sharedMemoryName, size_t messageSize, bool createBuffer) // if !createBuffer will wait indefinitely for shmem to be created or exit requested
        : m_sharedBufferSize(sizeof(CSharedQueueCore) + CSharedQueueCore::GetRequiredBufferSize(messageSize))
        , m_unlinkSharedBuffer(createBuffer)
        , m_pSharedBuffer(nullptr)
        , m_sharedMemoryName(sharedMemoryName)
    {
        bool sharedBufferWasCreated = false;

        if (createBuffer)
        {
            m_sharedMemoryFD = shm_open(m_sharedMemoryName.c_str(), O_RDWR, 0666);
            if (m_sharedMemoryFD == -1)
            {
                m_sharedMemoryFD = shm_open(m_sharedMemoryName.c_str(), O_CREAT | O_RDWR, 0666);
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
                m_sharedMemoryFD = shm_open(m_sharedMemoryName.c_str(), O_RDWR, 0666);
                retry = (m_sharedMemoryFD == -1) && (errno == ENOENT);
                if (retry)
                {
                    std::this_thread::sleep_for(PollInterval);  // for now... [TODO] make better wait
                }
            }while(retry && !CSignalHandlers::isExitRequested());
        }

        m_pSharedBuffer = static_cast<unsigned char *>(mmap(nullptr,
                                                            m_sharedBufferSize,
                                                            PROT_READ | PROT_WRITE,
                                                            MAP_SHARED,
                                                            m_sharedMemoryFD,
                                                            0
                                                            ));

        if (m_pSharedBuffer == MAP_FAILED)
            m_pSharedBuffer = nullptr;

        if ((m_pSharedBuffer != nullptr) && sharedBufferWasCreated)
        {
            new (m_pSharedBuffer) CSharedQueueCore(messageSize);
        }
    }

    ~CSharedQueue()
    {
        if (m_pSharedBuffer != nullptr)
            munmap(m_pSharedBuffer, m_sharedBufferSize);

        close(m_sharedMemoryFD);
        if (m_unlinkSharedBuffer)
        {
            unlinkBuffer();
        }
    }

    auto messageSize() const { return queue().messageSize(); }

    unsigned char *queueDataPtr() { return m_pSharedBuffer + QueueDataBufferOffset; }
    auto getPusher() { return queue().getPusher(); }
    auto getPopper() { return queue().getPopper(); }

    void unlinkBuffer() const { shm_unlink(m_sharedMemoryName.c_str()); }

private:
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
    std::string m_sharedMemoryName;
};
