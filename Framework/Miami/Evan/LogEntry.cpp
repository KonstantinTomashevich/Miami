#include <cassert>

#include <Miami/Evan/LogEntry.hpp>

namespace Miami::Evan
{
const char *GetLogLevelName (LogLevel level)
{
    switch (level)
    {
        case LogLevel::VERBOSE:
            return "VERBOSE";

        case LogLevel::DEBUG:
            return "DEBUG";

        case LogLevel::INFO:
            return "INFO";

        case LogLevel::WARNING:
            return "WARNING";

        case LogLevel::ERROR:
            return "ERROR";
    }

    assert(false);
    return "UNKNOWN";
}
}
