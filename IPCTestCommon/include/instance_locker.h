#pragma once

#include <string>
#include <sys/file.h>
#include <unistd.h>

class CInstanceLocker
{
    const std::string LockFilePath = "/tmp/";

public:
    CInstanceLocker(std::string instanceIdentifier)
        : m_lockFileName(LockFilePath + instanceIdentifier + ".lock")
    {
        m_lockFileFD = open(m_lockFileName.c_str(), O_CREAT | O_RDWR, 0666);
        m_instanceLocked = (flock(m_lockFileFD, LOCK_EX | LOCK_NB) == 0) || (errno != EWOULDBLOCK);
    }

    ~CInstanceLocker()
    {
        if (m_instanceLocked && (m_lockFileFD != -1))
        {
            close(m_lockFileFD);
            m_lockFileFD = -1;
            m_instanceLocked = false;
            remove(m_lockFileName.c_str());
        }
    }

    bool isLocked() const { return m_instanceLocked; }

private:
    bool m_instanceLocked = false;
    std::string m_lockFileName;
    int m_lockFileFD = -1;
};
