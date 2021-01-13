#pragma once

#include <cstdint>
#include <unordered_map>
#include <memory>

#include <Miami/Annotations.hpp>

#include <Miami/Disco/Disco.hpp>

#include <Miami/Hotline/ResultCode.hpp>

namespace Miami::Hotline
{
using EntryTypeId = uint64_t;

class Session final
{
public:
    explicit Session (Disco::Context *multithreadingContext);

    template <typename Entry>
    free_call ResultCode GetEntry (const std::shared_ptr <Disco::SafeLockGuard> &readOrWriteGuard,
                                   const Entry *&output) const;

    template <typename Entry>
    free_call ResultCode GetEntryForWrite (const std::shared_ptr <Disco::SafeLockGuard> &writeGuard, Entry *&output);

private:
    free_call bool CheckReadOrWriteGuard (const std::shared_ptr <Disco::SafeLockGuard> &readOrWriteGuard) const;

    free_call bool CheckWriteGuard (const std::shared_ptr <Disco::SafeLockGuard> &writeGuard) const;

    Disco::ReadWriteGuard guard_;
    // TODO: Use flat hash map.
    std::unordered_map <EntryTypeId, std::shared_ptr <void>> entries_;
};

template <typename Entry>
ResultCode
Session::GetEntry (const std::shared_ptr <Disco::SafeLockGuard> &readOrWriteGuard, const Entry *&output) const
{
    if (!CheckReadOrWriteGuard (readOrWriteGuard))
    {
        return ResultCode::INVARIANTS_VIOLATED;
    }

    auto iterator = entries_.find (Entry::TYPE_ID);
    if (iterator == entries_.end ())
    {
        output = nullptr;
    }
    else
    {
        output = reinterpret_cast <Entry *> (iterator->second.get ());
    }

    return ResultCode::OK;
}

template <typename Entry>
ResultCode Session::GetEntryForWrite (const std::shared_ptr <Disco::SafeLockGuard> &writeGuard, Entry *&output)
{
    if (!CheckWriteGuard (writeGuard))
    {
        return ResultCode::INVARIANTS_VIOLATED;
    }

    auto iterator = entries_.find (Entry::TYPE_ID);
    if (iterator == entries_.end ())
    {
        auto result = entries_.emplace (Entry::TYPE_ID, std::make_shared <Entry> ());
        if (result.second)
        {
            return ResultCode::OK;
        }
        else
        {
            Evan::Logger::Get ().Log (Evan::LogLevel::ERROR,
                                      "Unable to add entry to session, because emplace operation failed!");
            assert (false);

            output = nullptr;
            return ResultCode::INVARIANTS_VIOLATED;
        }
    }
    else
    {
        output = reinterpret_cast <Entry *> (iterator->second.get ());
        return ResultCode::OK;
    }
}
}
