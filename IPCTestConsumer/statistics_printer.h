#pragma once

#include <chrono>

#include "common.h"


class CStatisticsPrinter
{
private:
    constexpr static const auto PrintStatisticsInterval = std::chrono::seconds(1);

public:
    CStatisticsPrinter(size_t messageSize, TTimestamp initialTimestamp = MicrosecondsSinceEpoch())
        : m_messageSize(messageSize)
        , m_timestamp(initialTimestamp)
        , m_messagesReceived(0)
        , m_checksumMismatches(0)
        , m_outOfOrderMessages(0)
    {}

    void update(size_t totalMessagesReceived, TTimestamp newTimestamp = MicrosecondsSinceEpoch())
    {
        if ((newTimestamp - m_timestamp) >= PrintStatisticsInterval_us)
        {
            updateAndPrint(totalMessagesReceived, newTimestamp);
        }
    }

    void checksumMismatch() { ++m_checksumMismatches; }
    void outOfOrderMessage() { ++m_outOfOrderMessages; }

    void updateAndPrint(size_t totalMessagesReceived, TTimestamp newTimestamp = MicrosecondsSinceEpoch());

private:
    static const TTimestamp PrintStatisticsInterval_us = std::chrono::duration_cast<std::chrono::microseconds>(PrintStatisticsInterval).count();

    static TTimestamp MicrosecondsSinceEpoch()
    {
        using namespace std::chrono;

        microseconds us = duration_cast<microseconds>(system_clock::now().time_since_epoch());
        return us.count();
    }

    const size_t m_messageSize;
    TTimestamp m_timestamp;
    size_t m_messagesReceived;
    int m_checksumMismatches;
    int m_outOfOrderMessages;
};
