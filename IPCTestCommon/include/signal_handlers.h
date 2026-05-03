#pragma once

#include <atomic>
#include <csignal>

class CSignalHandlers
{
public:
    static bool isExitRequested(){ return exitRequested.load(std::memory_order_relaxed); }
    static bool isPauseRequested(){ return pauseRequested.load(std::memory_order_relaxed); }

private:
    CSignalHandlers()
    {
        std::signal(SIGINT, signal_handler);
        std::signal(SIGTERM, signal_handler);
        std::signal(SIGUSR1, signal_handler_pause);
    }

    static void signal_handler(int signal)
    {
        exitRequested.store(true, std::memory_order_relaxed);
    }

    static void signal_handler_pause(int signal)
    {
        const auto wasRequested = pauseRequested.load(std::memory_order_relaxed);
        pauseRequested.store(!wasRequested, std::memory_order_relaxed);
    }

    inline static std::atomic_bool exitRequested = false;
    inline static std::atomic_bool pauseRequested = false;

    static CSignalHandlers instance;
};

inline CSignalHandlers CSignalHandlers::instance;
