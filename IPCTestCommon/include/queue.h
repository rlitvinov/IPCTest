#pragma once

#include <atomic>
#include <cassert>
#include <functional>

class CSharedQueueCore // Single Producer Single Consumer Lock-free Queue
                       // inspired by this Charles Frasch lecture https://www.youtube.com/watch?v=K3P_Lmq6pw0
{
private:
    static const size_t QueueBufferSizeLog2 = 16;

    struct SIndicesCache
    {
        SIndicesCache(size_t begin, size_t end)
            :m_beginIndex(begin)
            ,m_endIndex(end)
        {}

        size_t m_beginIndex;
        size_t m_endIndex;
    };

public:
    static const size_t QueueBufferSize = 1 << QueueBufferSizeLog2;

    class CPusher: public SIndicesCache
    {
    public:
        CPusher(CSharedQueueCore &queue)
            : SIndicesCache(queue.m_beginIndex.load(std::memory_order_acquire), queue.m_endIndex.load(std::memory_order_relaxed))
            , m_queue(queue)
        {}

        void wait(std::function<bool()> interruptionRequested)
        {
            while (m_queue.full(m_beginIndex, m_endIndex) && !interruptionRequested()) // spinlock for performance; [TODO] maybe try other options...
                m_beginIndex = m_queue.m_beginIndex.load(std::memory_order_acquire);
        }

        size_t dataOffset() const
        {
            assert(!m_queue.full(m_beginIndex, m_endIndex));
            return m_endIndex & BufferIndexMask;
        }

        void push()
        {
            m_endIndex += m_queue.messageSize();
            m_queue.m_endIndex.store(m_endIndex, std::memory_order_release);
        }

    private:
        CSharedQueueCore &m_queue;
    };

    class CPopper: public SIndicesCache
    {
    public:
        CPopper(CSharedQueueCore &queue)
            : SIndicesCache(queue.m_beginIndex.load(std::memory_order_relaxed), queue.m_endIndex.load(std::memory_order_acquire))
            , m_queue(queue)
        {}

        void wait(std::function<bool()> interruptionRequested)
        {
            while (m_queue.empty(m_beginIndex, m_endIndex) && !interruptionRequested()) // spinlock for performance; [TODO] maybe try other options...
                m_endIndex = m_queue.m_endIndex.load(std::memory_order_acquire);
        }

        size_t dataOffset() const
        {
            assert(!m_queue.empty(m_beginIndex, m_endIndex));
            return m_beginIndex & BufferIndexMask;
        }

        void pop()
        {
            m_beginIndex += m_queue.messageSize();
            m_queue.m_beginIndex.store(m_beginIndex, std::memory_order_release);
        }

    private:
        CSharedQueueCore &m_queue;
    };


    CSharedQueueCore(size_t messageSize)
        : m_messageSize(messageSize)
    {}

    ~CSharedQueueCore() = delete; // this class cannot have destructor, because it "lives" in two different processes
                                  // and it is difficult to consistently call the destructor once

    size_t messageSize() const { return m_messageSize; }

    CPusher getPusher() { return CPusher(*this); }
    CPopper getPopper() { return CPopper(*this); }

    static size_t GetRequiredBufferSize(size_t messageSize) // additional space is needed to not worry about wrapping
    { return QueueBufferSize + messageSize - 1; }           // when message starts near the end of the buffer

private:
    static const size_t BufferIndexMask = QueueBufferSize - 1;

    bool full(size_t begin, size_t end) const { return (QueueBufferSize - end + begin) < m_messageSize; }
    static bool empty(size_t begin, size_t end) { return begin == end; }


    const size_t m_messageSize;
    alignas(std::hardware_destructive_interference_size) std::atomic<size_t> m_beginIndex = 0;
    alignas(std::hardware_destructive_interference_size) std::atomic<size_t> m_endIndex = 0;
    char m_padding[std::hardware_destructive_interference_size - sizeof(size_t)];

    static_assert(decltype(m_beginIndex)::is_always_lock_free);
};
