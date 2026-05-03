#include <algorithm>
#include <filesystem>
#include <iostream>

#include "common.h"
#include "instance_locker.h"
#include "shared_queue.h"

namespace
{
    const auto InstanceSemaphoreName = "IPCTestProducerInstance";
}


void FillPayload(unsigned char *pPayload, size_t payloadSize)
{
    static unsigned char counter = 0;

    std::generate(pPayload, pPayload + payloadSize, [](){ return counter++; });

    ++counter;
}

void loop(size_t payloadSize)
{
    auto messageSize = sizeof(SMessageHeader) + payloadSize;
    CSharedQueue sharedQueue(SharedMemoryName, messageSize, true);
    messageSize = sharedQueue.messageSize();
    {
        const auto actualPaylod = messageSize - sizeof(SMessageHeader);
        if (actualPaylod != payloadSize)
        {
            sharedQueue.unlinkBuffer();
            payloadSize = actualPaylod;
            std::cout << "Shared buffer already initialized and has paylod size different from specified.\n";
            std::cout << "Changing payload size in existing buffer is not supported (yet?), so using existing payload size.\n";
            std::cout << "For new payload size please exit both producer and consumer and restart them\n";
            std::cout << "Payload size == " << payloadSize << std::endl;
        }
    }

    auto pusher = sharedQueue.getPusher();

    for (size_t messageNumber = 0; ; ++messageNumber)
    {
        pusher.wait([](){ return CSignalHandlers::isExitRequested(); });

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

        auto pNextMessage = sharedQueue.queueDataPtr() + pusher.dataOffset();

        auto pHeader = reinterpret_cast<SMessageHeader*>(pNextMessage);
        auto pPayload = pNextMessage + sizeof(SMessageHeader);

        FillPayload(pPayload, payloadSize);
        pHeader->checksum = SMessageHeader::CalculateChecksum(pPayload, payloadSize);
        pHeader->sequenceNumber = messageNumber;
        pHeader->timeStamp = SMessageHeader::GenerateTimestamp();

        // [!] [debug]
        //if ((messageNumber & ((1 << 10) - 1)) == 0)
        //    pPayload[payloadSize - 1] = 1;

        pusher.push();
    }
}

int main(int argc, char** argv)
{
    CInstanceLocker instanceLocker(InstanceSemaphoreName);

    if (!instanceLocker.isLocked())
    {
        std::cout << "Only one instance of IPCTestProducer can be running" << std::endl;
        return 1;
    }

    std::cout << "Producer starting..." << std::endl;

    if (argc != 2)
    {
        const auto commandName = std::filesystem::path(argv[0]).stem().string();
        std::cout << "Usage: " << commandName << " <payload size in bytes>" << std::endl;
        return 0;
    }

    size_t payloadSize = 0;

    try
    {
        payloadSize = std::stoul(argv[1]);
    }
    catch(std::out_of_range)
    {
        std::cout << "Error: payload size is out of range." << std::endl;
        return 1;
    }
    catch(...)
    {
        std::cout << "Error: can't parse payload size" << std::endl;
        return 1;
    }

    std::cout << "Payload size == " << payloadSize << std::endl;

    if(const auto MessageSize = sizeof(SMessageHeader) + payloadSize;
       MessageSize > CSharedQueueCore::QueueBufferSize)
    {
        const auto MaxPayload = CSharedQueueCore::QueueBufferSize - sizeof(SMessageHeader);
        std::cout << "Payload must be between 0 and " << MaxPayload << std::endl;
        return 1;
    }

    loop(payloadSize);

    return 0;
}
