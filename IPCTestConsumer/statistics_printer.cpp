#include "statistics_printer.h"

#include <iostream>

void CStatisticsPrinter::updateAndPrint(size_t totalMessagesReceived, TTimestamp newTimestamp)
{
    const auto timeElapsed_us = static_cast<float>(newTimestamp - m_timestamp);
    const auto newMessages = totalMessagesReceived - m_messagesReceived;
    const auto messagesPerSecond = newMessages * 1000000 / timeElapsed_us;
    const auto bytesPerSecond = messagesPerSecond * m_messageSize;

    std::cout << totalMessagesReceived << " total messages received\n";
    std::cout << static_cast<size_t>(messagesPerSecond) << " messages/sec\n";
    std::cout << static_cast<size_t>(bytesPerSecond) << " bytes/sec\n";
    std::cout << std::endl;

    m_timestamp = newTimestamp;
    m_messagesReceived = totalMessagesReceived;
}
