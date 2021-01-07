#pragma once

#include <cstdint>
#include <vector>
#include <unordered_set>

#include <Miami/Disco/Annotations.hpp>
#include <Miami/Disco/Context.hpp>
#include <Miami/Disco/Variants.hpp>

namespace Miami::Disco
{
// TODO: Think about copy and move constructors.

class BaseLock
{
public:
    explicit BaseLock (Context *context);

    kernel_call ~BaseLock ();

    void RegisterLockGroup (const AnyLockGroupPointer &lockGroup, KernelModeGuard::RAII &kernelModeGuard);

    void UnregisterLockGroup (const AnyLockGroupPointer &lockGroup, KernelModeGuard::RAII &kernelModeGuard);

    void RegisterSafeGuard (SafeLockGuard *safeGuard, KernelModeGuard::RAII &kernelModeGuard);

    void UnregisterSafeGuard(SafeLockGuard *safeGuard, KernelModeGuard::RAII &kernelModeGuard);

    Context *GetContext () const;

protected:
    kernel_call void InformGroupsAboutUnblock (KernelModeGuard::RAII &kernelModeGuard);

private:
    Context *context_;

    // TODO: Use FlatHashSet?
    std::vector <AnyLockGroupPointer> dependantGroups_;

    // TODO: Use FlatHashSet?
    std::unordered_set<SafeLockGuard *> dependantGuards_;
};

class Lock final : public BaseLock
{
public:
    static constexpr LockType LOCK_TYPE = LockType::LOCK;

    explicit Lock (Context *context);

    kernel_call bool TryLock ();

    bool TryLock (KernelModeGuard::RAII &kernelModeGuard);

    kernel_call void Unlock ();

    void Unlock (KernelModeGuard::RAII &kernelModeGuard);

private:
    void Unlock (bool silently, KernelModeGuard::RAII &kernelModeGuard);

    bool isLocked_;

    friend class AnyLockPointer;
};

class ReadWriteGuard;

class ReadLock final : public BaseLock
{
public:
    static constexpr LockType LOCK_TYPE = LockType::READ_LOCK;

    kernel_call bool TryLock ();

    bool TryLock (KernelModeGuard::RAII &kernelModeGuard);

    kernel_call void Unlock ();

    void Unlock (KernelModeGuard::RAII &kernelModeGuard);

    ReadLock &operator = (const ReadLock &another) = delete;

private:
    explicit ReadLock (ReadWriteGuard *owner, Context *context);

    void Unlock (bool silently, KernelModeGuard::RAII &kernelModeGuard);

    ReadWriteGuard *owner_;

    friend class AnyLockPointer;

    friend class ReadWriteGuard;

    friend class WriteLock;
};

class WriteLock final : public BaseLock
{
public:
    static constexpr LockType LOCK_TYPE = LockType::WRITE_LOCK;

    kernel_call bool TryLock ();

    bool TryLock (KernelModeGuard::RAII &kernelModeGuard);

    kernel_call void Unlock ();

    void Unlock (KernelModeGuard::RAII &kernelModeGuard);

    WriteLock &operator = (const WriteLock &another) = delete;

private:
    explicit WriteLock (ReadWriteGuard *owner, Context *context);

    void Unlock (bool silently, KernelModeGuard::RAII &kernelModeGuard);

    ReadWriteGuard *owner_;

    friend class AnyLockPointer;

    friend class ReadWriteGuard;

    friend class ReadLock;
};

class ReadWriteGuard final
{
public:
    explicit ReadWriteGuard (Context *context);

    ReadLock &Read ();

    WriteLock &Write ();

private:
    ReadLock read_;
    WriteLock write_;

    uint32_t readersCount_;
    bool anyWriter_;

    friend class ReadLock;

    friend class WriteLock;
};
}
