#pragma once

#include <cstdint>
#include <vector>

#include <Miami/Annotations.hpp>

#include <Miami/Evan/LogEntry.hpp>

namespace Miami::Evan
{
class Logger final
{
public:
    static constexpr uint32_t MAX_BUFFER_SIZE = 25;

    static Logger &Get ();

    ~Logger();
    free_call void Log (LogLevel level, const std::string &message);

private:
    Logger () noexcept;

    // Warning: thread_local destructors are broken on modern mingw-w64.
    static thread_local Logger threadLocalLogger_;

    std::vector <LogEntry> buffer_;

    // TODO: Maybe add optional local output to file?
};
}
