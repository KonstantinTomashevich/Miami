#include <cassert>

#include <Miami/Evan/Logger.hpp>

#include <Miami/Hotline/Session.hpp>

namespace Miami::Hotline
{
Session::Session (Disco::Context *multithreadingContext)
    : guard_ (multithreadingContext),
      entries_ ()
{
}

bool Session::CheckReadOrWriteGuard (const std::shared_ptr <Disco::SafeLockGuard> &readOrWriteGuard) const
{
    if (Disco::IsReadOrWriteCaptured (readOrWriteGuard, guard_))
    {
        return true;
    }
    else
    {
        Evan::Logger::Get ().Log (Evan::LogLevel::ERROR,
                                  "Caught attempt to read session data without proper read or write guard!");
        assert (false);
        return false;
    }
}

bool Session::CheckWriteGuard (const std::shared_ptr <Disco::SafeLockGuard> &writeGuard) const
{
    if (Disco::IsWriteCaptured (writeGuard, guard_))
    {
        return true;
    }
    else
    {
        Evan::Logger::Get ().Log (Evan::LogLevel::ERROR,
                                  "Caught attempt to edit session data without proper write guard!");
        assert (false);
        return false;
    }
}
}