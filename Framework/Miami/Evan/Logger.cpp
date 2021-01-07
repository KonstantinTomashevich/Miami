#include <cassert>
#include <chrono>
#include <thread>

#include <Miami/Evan/Logger.hpp>
#include <Miami/Evan/GlobalLogger.hpp>

namespace Miami::Evan
{
Logger &Logger::Get ()
{
    thread_local Logger currentThreadLogger;
    return currentThreadLogger;
}

Logger::~Logger ()
{
    if (!buffer_.empty ())
    {
        GlobalLogger::Instance ().Flush (buffer_);
    }
}

void Logger::Log (LogLevel level, const std::string &message)
{
    assert(!message.empty ());
    buffer_.emplace_back (LogEntry {message, std::chrono::system_clock::now (), level});

    if (buffer_.size () >= MAX_BUFFER_SIZE)
    {
        GlobalLogger::Instance ().Flush (buffer_);
        buffer_.clear ();
    }
}
}
