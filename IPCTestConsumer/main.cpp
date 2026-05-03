#include <iostream>

#include "common.h"
#include "instance_locker.h"
#include "shared_queue.h"
#include "signal_handlers.h"
#include "statistics_printer.h"

namespace
{
    const auto InstanceSemaphoreName = "IPCTestConsumerInstance";
}

void loop()
{
    CSharedQueue sharedQueue(SharedMemoryName, 0, false);

    if (CSignalHandlers::isExitRequested())
        return;

    const auto MessageSize = sharedQueue.messageSize();
    const auto PayloadSize = MessageSize - sizeof(SMessageHeader);

    std::cout << "PayloadSize: " << PayloadSize << std::endl;

    TSequenceNumber prevSequenceNumber = 0;

    CStatisticsPrinter statPrinter(MessageSize);

    auto popper = sharedQueue.getPopper();

    for (size_t messageCount = 0; ; ++messageCount)
    {
        popper.wait([](){ return CSignalHandlers::isExitRequested(); });

        if (CSignalHandlers::isPauseRequested())
        {
            std::cout << "Pausing..." << std::endl;
            while (CSignalHandlers::isPauseRequested() && !CSignalHandlers::isExitRequested())
            {
                std::this_thread::sleep_for(PollInterval); // [TODO] make better wait (conditional variable?)
            }
            std::cout << "Resuming..." << std::endl;
        }

        if (CSignalHandlers::isExitRequested())
        {
            std::cout << "Exiting..." << std::endl;
            break;
        }

        const auto pFrontMessage = sharedQueue.queueDataPtr() + popper.dataOffset();

        const auto pHeader = reinterpret_cast<SMessageHeader*>(pFrontMessage);
        const auto pPayload = pFrontMessage + sizeof(SMessageHeader);

        if (auto calculatedChecksum = SMessageHeader::CalculateChecksum(pPayload, PayloadSize);
            calculatedChecksum != pHeader->checksum)
        {
            statPrinter.checksumMismatch();
        }

        if ((messageCount > 0) && (prevSequenceNumber != pHeader->sequenceNumber - 1))
        {
            statPrinter.outOfOrderMessage();
        }

        statPrinter.update(messageCount);

        prevSequenceNumber = pHeader->sequenceNumber;

        popper.pop();
    }
}

int main()
{
    CInstanceLocker instanceLocker(InstanceSemaphoreName);

    if (!instanceLocker.isLocked())
    {
        std::cout << "Only one instance of IPCTestConsumer can be running" << std::endl;
        return 1;
    }

    std::cout << "Consumer starting..." << std::endl;
    loop();
    return 0;
}
