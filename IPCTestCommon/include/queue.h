#pragma once

#include <atomic>
#include <cassert>

class CSharedQueueCore // Single Producer Single Consumer Lock-free Queue
                       // inspired by this Charles Frasch lecture https://www.youtube.com/watch?v=K3P_Lmq6pw0
{
private:
    static const size_t QueueBufferSizeLog2 = 20;

public:
    static const size_t QueueBufferSize = 1 << QueueBufferSizeLog2;

    CSharedQueueCore(size_t messageSize)
        : m_messageSize(messageSize)
    {}

    ~CSharedQueueCore() = delete; // this class cannot have destructor, because it "lives" in two different processes
                                  // and it is difficult to consistently call the destructor once

    auto messageSize() const { return m_messageSize; }
    auto size() const { return m_endIndex - m_beginIndex; }
    auto empty() const { return size() == 0; }
    auto full() const { return (QueueBufferSize - size()) < m_messageSize; }

    auto frontMessageOffset() const // offset of the front message in the buffer; queue must not be empty
    {
        assert(!empty());
        return m_beginIndex & BufferIndexMask;
    }

    auto nextMessageOffset() // offset in the buffer of the next message to be added to the queue by push_back()
                             // queue must not be full
    {
        assert(!full());
        return m_endIndex & BufferIndexMask;
    }

    void pop_front() // deletes message from the front of the queue
    {
        assert(!empty());
        m_beginIndex += m_messageSize;
    }

    void push_back() // commits (adds to the end of the queue) next message
                     // filled in the buffer at offset returned by nextMessageOffset()
    {
        assert(!full());
        m_endIndex += m_messageSize;
    }

    static size_t GetRequiredBufferSize(size_t messageSize) // additional space is needed to not worry about wrapping
    { return QueueBufferSize + messageSize - 1; }           // when message starts near the end of the buffer

private:
    static const size_t BufferIndexMask = QueueBufferSize - 1;

    const size_t m_messageSize;
    std::atomic<size_t> m_beginIndex = 0;
    std::atomic<size_t> m_endIndex = 0;

    static_assert(decltype(m_beginIndex)::is_always_lock_free);
};
