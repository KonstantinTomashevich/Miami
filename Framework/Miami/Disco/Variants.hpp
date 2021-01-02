#pragma once

#include <vector>

#include <Miami/Disco/Context.hpp>

namespace Miami::Disco
{
class Lock;

class ReadLock;

class WriteLock;

class SafeLockGuard;

enum class LockType
{
    LOCK,
    READ_LOCK,
    WRITE_LOCK
};

enum class LockGroupType
{
    ONE,
    MULTIPLE
};

class AnyLockGroupPointer final
{

};

class AnyLockPointer final
{
public:
    template <LockType lockType, typename LockClass>
    explicit AnyLockPointer (LockClass *lock);

    kernel_call bool TryLock () const;

    bool TryLock (KernelModeGuard::RAII &kernelModeGuard) const;

    kernel_call void Unlock () const;

    void Unlock (KernelModeGuard::RAII &kernelModeGuard) const;

    void RegisterLockGroup (const AnyLockGroupPointer &lockGroup, KernelModeGuard::RAII &kernelModeGuard);

    void UnregisterLockGroup (const AnyLockGroupPointer &lockGroup, KernelModeGuard::RAII &kernelModeGuard);

    void RegisterSafeGuard (SafeLockGuard *safeGuard, KernelModeGuard::RAII &kernelModeGuard);

    void UnregisterSafeGuard(SafeLockGuard *safeGuard, KernelModeGuard::RAII &kernelModeGuard);

    Context *GetContext () const;

    void Nullify ();

    LockType GetType () const;

    friend bool TryLockAll (const std::vector <AnyLockPointer> &locks, KernelModeGuard::RAII &kernelModeGuard);

private:
    void Unlock (bool silently, KernelModeGuard::RAII &kernelModeGuard) const;

    LockType lockType_;
    void *lock_;
};

class SafeLockGuard final
{
public:
    SafeLockGuard (const SafeLockGuard &another) = delete;
    /// Movement is deleted, because it is very ineffective due to kernel call requirement.
    SafeLockGuard (SafeLockGuard &&another) = delete;
    kernel_call ~SafeLockGuard();

private:
    /// This type of guard can be constructed only in special cases (for example, by lock groups).
    SafeLockGuard (const AnyLockPointer &lockPointer, KernelModeGuard::RAII &kernelModeGuard);

    void Invalidate ();

    AnyLockPointer pointer_;

    friend class BaseLock;
    friend class OneLockGroup;
    friend class MultipleLockGroup;
};

template <LockType lockType, typename LockClass>
AnyLockPointer::AnyLockPointer (LockClass *lock)
    : lockType_ (lockType), lock_ (lock)
{
    static_assert (lockType == LockClass::LOCK_TYPE, "Found lock type and lock class mismatch!");
}
}
