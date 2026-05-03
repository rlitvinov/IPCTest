#pragma once

#include <chrono>
#include <string_view>
#include <numeric>

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
        //return std::hash<std::string_view>()(std::string_view(reinterpret_cast<char*>(pPayload), payloadSize));
        return std::accumulate(pPayload, pPayload + payloadSize, 0);// checksum calculation is significantly impacting performance
                                                                    // so reverted to the literal sum
    }

    static TTimestamp GenerateTimestamp() // timestamp generation was taking significant time, especially for small payloads,
                                          // so changed to rdtsc
    {
        return __rdtsc();
    }
};
