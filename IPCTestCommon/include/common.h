#pragma once

#include <chrono>
#include <string_view>


const auto PollInterval = std::chrono::microseconds(10);
const auto SharedMemoryName = "IPCTestQueueSharedBuffer";

typedef int64_t TTimestamp;
typedef int64_t TSequenceNumber;
typedef int64_t TChecksum;

struct __attribute__((packed)) SMessageHeader
{
    TTimestamp      timeStamp;
    TSequenceNumber sequenceNumber;
    TChecksum       checksum;

    static TChecksum CalculateChecksum(unsigned char *pPayload, size_t payloadSize)
    {
        return std::hash<std::string_view>()(std::string_view(reinterpret_cast<char*>(pPayload), payloadSize)); // [TODO] Maybe try something faster...
    }

    static TTimestamp GenerateTimestamp() // microseconds since the epoch
    {
        using namespace std::chrono;

        microseconds us = duration_cast<microseconds>(system_clock::now().time_since_epoch());
        return us.count();
    }
};
