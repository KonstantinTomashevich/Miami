#include <cassert>

#include <Miami/Disco/Variants.hpp>
#include <Miami/Disco/Locks.hpp>
#include <Miami/Disco/LockGroups.hpp>

namespace Miami::Disco
{
// TODO: Same ideas as with CALL_LOCK_METHOD.
#define CALL_LOCK_GROUP_METHOD(MethodName, Default, Args...)                                                \
if (group_)                                                                                                 \
{                                                                                                           \
    switch (groupType_)                                                                                     \
    {                                                                                                       \
        case LockGroupType::ONE: return static_cast<OneLockGroup *> (group_)->MethodName (Args);            \
        case LockGroupType::MULTIPLE: return static_cast<MultipleLockGroup *> (group_)->MethodName (Args);  \
    }                                                                                                       \
}                                                                                                           \
                                                                                                            \
assert(!group_);                                                                                            \
return Default;

// TODO: Instead of using switch, create static array?
#define CALL_LOCK_METHOD(MethodName, Default, Args...)                                         \
if (lock_)                                                                                     \
{                                                                                              \
    switch (lockType_)                                                                         \
    {                                                                                          \
        case LockType::LOCK: return static_cast<Lock *> (lock_)->MethodName (Args);            \
        case LockType::READ_LOCK: return static_cast<ReadLock *> (lock_)->MethodName (Args);   \
        case LockType::WRITE_LOCK: return static_cast<WriteLock *> (lock_)->MethodName (Args); \
    }                                                                                          \
}                                                                                              \
                                                                                               \
assert(!lock_);                                                                                \
return Default;

bool AnyLockGroupPointer::operator == (const AnyLockGroupPointer &other) const
{
    return group_ == other.group_;
}

bool AnyLockGroupPointer::operator != (const AnyLockGroupPointer &other) const
{
    return !(other == *this);
}

bool AnyLockGroupPointer::TryCapture (KernelModeGuard::RAII &kernelModeGuard)
{
    CALL_LOCK_GROUP_METHOD(TryCapture, false, kernelModeGuard)
}

void AnyLockGroupPointer::Invalidate (void *invalidationSourceLock, KernelModeGuard::RAII &kernelModeGuard)
{
    CALL_LOCK_GROUP_METHOD(Invalidate, , invalidationSourceLock, kernelModeGuard)
}

bool AnyLockPointer::TryLock () const
{
    CALL_LOCK_METHOD(TryLock, false)
}

bool AnyLockPointer::TryLock (KernelModeGuard::RAII &kernelModeGuard) const
{
    CALL_LOCK_METHOD(TryLock, false, kernelModeGuard)
}

void AnyLockPointer::Unlock () const
{
    CALL_LOCK_METHOD(Unlock,)
}

void AnyLockPointer::Unlock (KernelModeGuard::RAII &kernelModeGuard) const
{
    CALL_LOCK_METHOD(Unlock, , kernelModeGuard)
}

void AnyLockPointer::Unlock (bool silently, KernelModeGuard::RAII &kernelModeGuard) const
{
    CALL_LOCK_METHOD(Unlock, , silently, kernelModeGuard)
}

void AnyLockPointer::RegisterLockGroup (const AnyLockGroupPointer &lockGroup, KernelModeGuard::RAII &kernelModeGuard)
{
    CALL_LOCK_METHOD(RegisterLockGroup, , lockGroup, kernelModeGuard)
}

void AnyLockPointer::UnregisterLockGroup (const AnyLockGroupPointer &lockGroup, KernelModeGuard::RAII &kernelModeGuard)
{
    CALL_LOCK_METHOD(UnregisterLockGroup, , lockGroup, kernelModeGuard)
}

void AnyLockPointer::RegisterSafeGuard (SafeLockGuard *safeGuard, KernelModeGuard::RAII &kernelModeGuard)
{
    CALL_LOCK_METHOD(RegisterSafeGuard, , safeGuard, kernelModeGuard)
}

void AnyLockPointer::UnregisterSafeGuard (SafeLockGuard *safeGuard, KernelModeGuard::RAII &kernelModeGuard)
{
    CALL_LOCK_METHOD(UnregisterSafeGuard, , safeGuard, kernelModeGuard)
}

Context *AnyLockPointer::GetContext () const
{
    CALL_LOCK_METHOD(GetContext, nullptr)
}

void AnyLockPointer::Nullify ()
{
    lock_ = nullptr;
}

bool AnyLockPointer::Is (void *raw) const
{
    return lock_ == raw;
}

bool AnyLockPointer::IsNull () const
{
    return lock_;
}

bool TryLockAll (const std::vector <AnyLockPointer> &locks, KernelModeGuard::RAII &kernelModeGuard)
{
    auto lockIterator = locks.begin ();
    while (lockIterator != locks.end ())
    {
        if (lockIterator->TryLock (kernelModeGuard))
        {
            ++lockIterator;
        }
        else
        {
            auto unlockIterator = locks.begin ();
            while (unlockIterator != lockIterator)
            {
                unlockIterator->Unlock (true, kernelModeGuard);
                ++unlockIterator;
            }
        }
    }

    return true;
}

SafeLockGuard::~SafeLockGuard ()
{
    if (pointer_.GetContext ())
    {
        KernelModeGuard::RAII guard = pointer_.GetContext ()->KernelMode ().Enter ();
        pointer_.Unlock (guard);
        pointer_.UnregisterSafeGuard (this, guard);
    }
}

SafeLockGuard::SafeLockGuard (const AnyLockPointer &lockPointer, KernelModeGuard::RAII &kernelModeGuard)
    : pointer_ (lockPointer)
{
    assert(kernelModeGuard.IsValid ());
    assert(!pointer_.IsNull ());
    pointer_.RegisterSafeGuard (this, kernelModeGuard);
}

void SafeLockGuard::Invalidate ()
{
    pointer_.Nullify ();
}
}