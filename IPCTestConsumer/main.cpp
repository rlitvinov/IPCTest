#include <iostream>

#include "common.h"
#include "shared_queue.h"
#include "statistics_printer.h"

void loop()
{
    CSharedQueue sharedQueue(0, false);

    const auto MessageSize = sharedQueue.messageSize();
    const auto PayloadSize = MessageSize - sizeof(SMessageHeader);

    std::cout << "PayloadSize: " << PayloadSize << std::endl;

    TSequenceNumber prevSequenceNumber = 0;

    CStatisticsPrinter statPrinter(MessageSize);

    for (size_t messageCount = 0; ; ++messageCount)
    {
        while(sharedQueue.empty()){} // spinlock for performance; [TODO] maybe try other options...
        const auto pFrontMessage = sharedQueue.frontMessagePtr();

        const auto pHeader = reinterpret_cast<SMessageHeader*>(pFrontMessage);
        const auto pPayload = pFrontMessage + sizeof(SMessageHeader);

        if (auto calculatedChecksum = SMessageHeader::CalculateChecksum(pPayload, PayloadSize);
            calculatedChecksum != pHeader->checksum)
        {
            std::cout << "Checksum mismatch: header: " << pHeader->checksum << ", calculated: " << calculatedChecksum << std::endl;
        }

        if ((messageCount > 0) && (prevSequenceNumber != pHeader->sequenceNumber - 1))
        {
            std::cout << "Sequence number out of order: " << prevSequenceNumber << ", " << pHeader->sequenceNumber << std::endl;
        }

        statPrinter.update(messageCount);

        prevSequenceNumber = pHeader->sequenceNumber;

        sharedQueue.pop_front();
    }
}

int main()
{
    std::cout << "Consumer starting..." << std::endl;
    loop();
    return 0;
}
