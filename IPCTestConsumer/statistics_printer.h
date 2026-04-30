#pragma once

#include <chrono>

#include "common.h"


class CStatisticsPrinter
{
private:
    constexpr static const auto PrintStatisticsInterval = std::chrono::seconds(1);

public:
    CStatisticsPrinter(size_t messageSize, TTimestamp initialTimestamp = SMessageHeader::GenerateTimestamp())
        : m_messageSize(messageSize)
        , m_timestamp(initialTimestamp)
        , m_messagesReceived(0)
    {}

    void update(size_t totalMessagesReceived, TTimestamp newTimestamp = SMessageHeader::GenerateTimestamp())
    {
        if ((newTimestamp - m_timestamp) >= PrintStatisticsInterval_us)
        {
            updateAndPrint(totalMessagesReceived, newTimestamp);
        }
    }

    void updateAndPrint(size_t totalMessagesReceived, TTimestamp newTimestamp = SMessageHeader::GenerateTimestamp());

private:
    static const TTimestamp PrintStatisticsInterval_us = std::chrono::duration_cast<std::chrono::microseconds>(PrintStatisticsInterval).count();

    const size_t m_messageSize;
    TTimestamp m_timestamp;
    size_t m_messagesReceived;
};
