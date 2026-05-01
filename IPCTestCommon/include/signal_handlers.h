#pragma once

#include <atomic>
#include <csignal>

class CSignalHandlers
{
public:
    static bool isExitRequested(){ return exitRequested; }
    static bool isPauseRequested(){ return pauseRequested; }

private:
    CSignalHandlers()
    {
        std::signal(SIGINT, signal_handler);
        std::signal(SIGTERM, signal_handler);
        std::signal(SIGUSR1, signal_handler_pause);
    }

    static void signal_handler(int signal)
    {
        exitRequested = true;
    }

    static void signal_handler_pause(int signal)
    {
        pauseRequested = !pauseRequested;
    }

    inline static volatile std::atomic_bool exitRequested = false;
    inline static volatile std::atomic_bool pauseRequested = false;

    static CSignalHandlers instance;
};

inline CSignalHandlers CSignalHandlers::instance;
