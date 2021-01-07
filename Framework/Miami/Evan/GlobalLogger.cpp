#include <cassert>
#include <thread>

#include <Miami/Evan/GlobalLogger.hpp>

namespace Miami::Evan
{
GlobalLogger GlobalLogger::instance_ {};

GlobalLogger &GlobalLogger::Instance ()
{
    return instance_;
}

void GlobalLogger::AddOutput (const std::shared_ptr <std::ostream> &output, LogLevel minLevel)
{
    assert(output);
    std::unique_lock <std::mutex> lock {guard_};
    outputs_.emplace_back (Output {output, minLevel});

    if (minLevel < minAcceptedLevel_)
    {
        minAcceptedLevel_ = minLevel;
    }
}

GlobalLogger::GlobalLogger () noexcept
    : guard_ (),
      outputs_ (),
      minAcceptedLevel_ (LogLevel::ERROR)
{
}

void GlobalLogger::Flush (const std::vector <LogEntry> &entries)
{
    // TODO: Currently global logger output is not strongly sorted by time. Is it worth it to somehow sort this output?
    assert(!entries.empty ());
    std::unique_lock <std::mutex> lock {guard_};
    std::thread::id threadId = std::this_thread::get_id ();

    for (const LogEntry &entry : entries)
    {
        if (entry.level_ >= minAcceptedLevel_)
        {
            time_t time = std::chrono::system_clock::to_time_t (entry.creationTime);
            // TODO: Output milliseconds?
            char *timeString = ctime (&time);
            // Temporary adhok to remove newline from ctime output.
            timeString[24] = '\0';

            for (Output &output : outputs_)
            {
                assert(output.output_);
                if (entry.level_ >= output.minLevel_ && output.output_)
                {
                    (*output.output_) << "[thread " << threadId << "] [" << timeString << "] " <<
                                      LogLevelName (entry.level_) << " " << entry.content_ << std::endl;
                }
            }
        }
    }
}
}
