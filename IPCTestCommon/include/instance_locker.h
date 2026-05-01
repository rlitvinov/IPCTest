#pragma once

#include <fcntl.h>
#include <semaphore.h>
#include <string>

class CInstanceLocker
{
public:
    CInstanceLocker(std::string instanceIdentifier)
        : m_instanceIdentifier(instanceIdentifier)
    {
        m_pInstanceSemaphore = sem_open(instanceIdentifier.c_str(), O_CREAT, 0666, 1);
        m_instanceSemaphoreLocked = (sem_trywait(m_pInstanceSemaphore) != -1);
    }

    ~CInstanceLocker()
    {
        if (m_instanceSemaphoreLocked)
        {
            sem_post(m_pInstanceSemaphore);
            sem_unlink(m_instanceIdentifier.c_str());
        }
        sem_close(m_pInstanceSemaphore);
    }

    bool isLocked() const { return m_instanceSemaphoreLocked; }

private:
    sem_t* m_pInstanceSemaphore = nullptr;
    bool m_instanceSemaphoreLocked = false;
    std::string m_instanceIdentifier;
};
