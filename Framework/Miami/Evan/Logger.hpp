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
    static Logger &Get ();

    ~Logger();
    free_call void Log (LogLevel level, const std::string &message);

private:
    Logger () = default;

    static constexpr uint32_t MAX_BUFFER_SIZE = 25;

    std::vector <LogEntry> buffer_;
};
}
