#pragma once

#include <mutex>
#include <vector>
#include <memory>
#include <ostream>

#include <Miami/Annotations.hpp>

#include <Miami/Evan/LogEntry.hpp>

namespace Miami::Evan
{
class GlobalLogger
{
public:
    static GlobalLogger &Instance ();

    void AddOutput (LogLevel minLevel, moved_in std::unique_ptr <std::ostream> &output_);

private:
    struct Output
    {
        std::unique_ptr <std::ostream> output_;
        LogLevel minLevel_;
    };

    GlobalLogger () noexcept;

    void Flush (const std::vector <LogEntry> &entries);

    static GlobalLogger instance_;

    std::mutex guard_;
    std::vector <Output> outputs_;
    LogLevel minAcceptedLevel_;

    friend class Logger;
};
}