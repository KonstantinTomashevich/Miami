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
    LOCK = 0,
    READ_LOCK,
    WRITE_LOCK
};

enum class LockGroupType
{
    ONE = 0,
    MULTIPLE
};

// TODO: Same ideas as with AnyLockPointer.
class AnyLockGroupPointer final
{
public:
    template <typename LockGroupClass>
    explicit AnyLockGroupPointer (LockGroupClass *group);

    bool operator == (const AnyLockGroupPointer &other) const;

    bool operator != (const AnyLockGroupPointer &other) const;

private:
    bool TryCapture (void *captureSourceLock, KernelModeGuard::RAII &kernelModeGuard);

    void Invalidate (void *invalidationSourceLock, KernelModeGuard::RAII &kernelModeGuard);

    LockGroupType groupType_;
    void *group_;

    friend class BaseLock;
};

// TODO: Use first N bytes in lock instance as identifier (like in virtuals) instead of #lockType_?
class AnyLockPointer final
{
public:
    template <typename LockClass>
    explicit AnyLockPointer (LockClass *lock);

    kernel_call bool TryLock () const;

    bool TryLock (KernelModeGuard::RAII &kernelModeGuard) const;

    kernel_call void Unlock () const;

    void Unlock (KernelModeGuard::RAII &kernelModeGuard) const;

    void RegisterLockGroup (const AnyLockGroupPointer &lockGroup, KernelModeGuard::RAII &kernelModeGuard);

    void UnregisterLockGroup (const AnyLockGroupPointer &lockGroup, KernelModeGuard::RAII &kernelModeGuard);

    void RegisterSafeGuard (SafeLockGuard *safeGuard, KernelModeGuard::RAII &kernelModeGuard);

    void UnregisterSafeGuard (SafeLockGuard *safeGuard, KernelModeGuard::RAII &kernelModeGuard);

    Context *GetContext () const;

    void Nullify ();

    bool Is (void *raw) const;

    bool IsNull () const;

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

    kernel_call ~SafeLockGuard ();

    bool Is (void *raw) const;

private:
    /// This type of guard can be constructed only in special cases (for example, by lock groups).
    SafeLockGuard (const AnyLockPointer &lockPointer, KernelModeGuard::RAII &kernelModeGuard);

    void Invalidate ();

    AnyLockPointer pointer_;

    friend class BaseLock;

    friend class OneLockGroup;

    friend class MultipleLockGroup;
};

template <typename LockClass>
AnyLockPointer::AnyLockPointer (LockClass *lock)
    : lockType_ (LockClass::LOCK_TYPE),
      lock_ (lock)
{
}

template <typename LockGroupClass>
AnyLockGroupPointer::AnyLockGroupPointer (LockGroupClass *group)
    : groupType_ (LockGroupClass::LOCK_GROUP_TYPE),
      group_ (group)
{
}
}
