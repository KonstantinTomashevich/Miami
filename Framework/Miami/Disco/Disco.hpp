#pragma once

#include <Miami/Disco/Annotations.hpp>
#include <Miami/Disco/Context.hpp>
#include <Miami/Disco/Variants.hpp>
#include <Miami/Disco/Locks.hpp>
#include <Miami/Disco/LockGroups.hpp>

namespace Miami::Disco
{
void After (const AnyLockPointer &lock, OneLockGroup::NextLambda next,
            OneLockGroup::CancelLambda cancel = nullptr);

void After (const std::vector <AnyLockPointer> &locks, MultipleLockGroup::NextLambda next,
            MultipleLockGroup::CancelLambda cancel = nullptr);

bool IsReadOrWriteCaptured (
    const std::shared_ptr <Disco::SafeLockGuard> &readOrWriteGuard, const ReadWriteGuard &guard);

bool IsWriteCaptured (
    const std::shared_ptr <Disco::SafeLockGuard> &writeGuard, const ReadWriteGuard &guard);

template <typename Lock>
void After (Lock *lock, OneLockGroup::NextLambda next,
            OneLockGroup::CancelLambda cancel = nullptr)
{
    After (AnyLockPointer (lock), std::move (next), std::move (cancel));
}
}
