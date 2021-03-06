#pragma once

#include <chrono>
#include <string>

namespace Miami::Evan
{
enum class LogLevel
{
    VERBOSE = 0,
    DEBUG,
    INFO,
    WARNING,
    ERROR
};

const char *GetLogLevelName (LogLevel level);

struct LogEntry
{
    std::string content_;
    std::chrono::time_point<std::chrono::system_clock> creationTime_;
    LogLevel level_;
};
}
