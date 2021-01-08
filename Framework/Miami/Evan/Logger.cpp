#include <cassert>

#include <Miami/Evan/Logger.hpp>
#include <Miami/Evan/GlobalLogger.hpp>

namespace Miami::Evan
{
thread_local Logger Logger::threadLocalLogger_ {};

Logger &Logger::Get ()
{
    return threadLocalLogger_;
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

Logger::Logger () noexcept
    : buffer_ ()
{
}
}
