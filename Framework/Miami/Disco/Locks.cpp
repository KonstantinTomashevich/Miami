#include <algorithm>
#include <cassert>

#include <Miami/Disco/Locks.hpp>

namespace Miami::Disco
{
#define TRY_CALL_WITH_KERNEL_MODE(MethodName) \
if (GetContext ()) \
{ \
    KernelModeGuard::RAII guard = GetContext ()->KernelMode ().Enter (); \
    return MethodName(guard); \
}

#define OTHERWISE(value) else return value;

BaseLock::BaseLock (Context *context)
    : context_ (context)
{
    assert(context_);
}

BaseLock::~BaseLock ()
{
    if (context_)
    {
        KernelModeGuard::RAII guard = context_->KernelMode ().Enter ();
        for (AnyLockGroupPointer group : dependantGroups_)
        {
            group.Invalidate (guard);
        }

        for (SafeLockGuard *safeGuard : dependantGuards_)
        {
            assert(safeGuard);
            safeGuard->Invalidate ();
        }
    }
}

void BaseLock::RegisterLockGroup (const AnyLockGroupPointer &lockGroup, KernelModeGuard::RAII &kernelModeGuard)
{
    assert(kernelModeGuard.IsValid ());
    assert(std::find (dependantGroups_.begin (), dependantGroups_.end (), lockGroup) == dependantGroups_.end ());
    dependantGroups_.push_back (lockGroup);
}

void BaseLock::UnregisterLockGroup (const AnyLockGroupPointer &lockGroup, KernelModeGuard::RAII &kernelModeGuard)
{
    assert(kernelModeGuard.IsValid ());
    auto iterator = std::find (dependantGroups_.begin (), dependantGroups_.end (), lockGroup);

    if (iterator != dependantGroups_.end ())
    {
        dependantGroups_.erase (iterator);
    }
}

void BaseLock::RegisterSafeGuard (SafeLockGuard *safeGuard, KernelModeGuard::RAII &kernelModeGuard)
{
    assert(kernelModeGuard.IsValid ());
    assert(safeGuard);
    assert(dependantGuards_.count (safeGuard) == 0);
    dependantGuards_.insert (safeGuard);
}

void BaseLock::UnregisterSafeGuard (SafeLockGuard *safeGuard, KernelModeGuard::RAII &kernelModeGuard)
{
    assert(kernelModeGuard.IsValid ());
    assert(safeGuard);
    assert(dependantGuards_.count (safeGuard) == 1);
    dependantGuards_.erase (safeGuard);
}

Context *BaseLock::GetContext () const
{
    return context_;
}

void BaseLock::InformGroupsAboutUnblock (KernelModeGuard::RAII &kernelModeGuard)
{
    assert(kernelModeGuard.IsValid ());
    for (auto iterator = dependantGroups_.begin (); iterator != dependantGroups_.end (); ++iterator)
    {
        if (iterator->TryCapture (kernelModeGuard))
        {
            dependantGroups_.erase (iterator);
            return;
        }
    }
}

Lock::Lock (Context *context)
    : BaseLock (context),
      isLocked_ (false)
{
}

bool Lock::TryLock ()
{
    TRY_CALL_WITH_KERNEL_MODE(TryLock) OTHERWISE(false)
}

bool Lock::TryLock (KernelModeGuard::RAII &kernelModeGuard)
{
    assert(kernelModeGuard.IsValid ());
    if (isLocked_)
    {
        return false;
    }
    else
    {
        isLocked_ = true;
        return true;
    }
}

void Lock::Unlock ()
{
    TRY_CALL_WITH_KERNEL_MODE(Unlock)
}

void Lock::Unlock (KernelModeGuard::RAII &kernelModeGuard)
{
    Unlock (false, kernelModeGuard);
}

void Lock::Unlock (bool silently, KernelModeGuard::RAII &kernelModeGuard)
{
    assert(kernelModeGuard.IsValid ());
    assert(isLocked_);
    isLocked_ = false;

    if (!silently)
    {
        InformGroupsAboutUnblock (kernelModeGuard);
    }
}

bool ReadLock::TryLock ()
{
    TRY_CALL_WITH_KERNEL_MODE(TryLock) OTHERWISE(false)
}

bool ReadLock::TryLock (KernelModeGuard::RAII &kernelModeGuard)
{
    assert(kernelModeGuard.IsValid ());
    if (owner_ && !owner_->anyWriter_)
    {
        ++owner_->readersCount_;
        return true;
    }
    else
    {
        return false;
    }
}

void ReadLock::Unlock ()
{
    TRY_CALL_WITH_KERNEL_MODE(Unlock)
}

void ReadLock::Unlock (KernelModeGuard::RAII &kernelModeGuard)
{
    Unlock (false, kernelModeGuard);
}

ReadLock::ReadLock (ReadWriteGuard *owner, Context *context)
    : BaseLock (context),
      owner_ (owner)
{
    assert(owner);
}

void ReadLock::Unlock (bool silently, KernelModeGuard::RAII &kernelModeGuard)
{
    assert(kernelModeGuard.IsValid ());
    if (owner_)
    {
        assert(owner_->readersCount_ > 0);
        --owner_->readersCount_;

        if (!silently)
        {
            InformGroupsAboutUnblock (kernelModeGuard);
        }
    }
}

bool WriteLock::TryLock ()
{
    TRY_CALL_WITH_KERNEL_MODE(TryLock) OTHERWISE(false)
}

bool WriteLock::TryLock (KernelModeGuard::RAII &kernelModeGuard)
{
    assert(kernelModeGuard.IsValid ());
    if (owner_ && !owner_->anyWriter_ && owner_->readersCount_ == 0)
    {
        owner_->anyWriter_ = true;
        return true;
    }
    else
    {
        return false;
    }
}

void WriteLock::Unlock ()
{
    TRY_CALL_WITH_KERNEL_MODE(Unlock)
}

void WriteLock::Unlock (KernelModeGuard::RAII &kernelModeGuard)
{
    Unlock (false, kernelModeGuard);
}

WriteLock::WriteLock (ReadWriteGuard *owner, Context *context)
    : BaseLock (context),
      owner_ (owner)
{
    assert(owner);
}

void WriteLock::Unlock (bool silently, KernelModeGuard::RAII &kernelModeGuard)
{
    assert(kernelModeGuard.IsValid ());
    if (owner_)
    {
        assert(owner_->anyWriter_);
        owner_->anyWriter_ = false;

        if (!silently)
        {
            InformGroupsAboutUnblock (kernelModeGuard);
        }
    }
}

ReadWriteGuard::ReadWriteGuard (Context *context)
    : read_ (this, context),
      write_ (this, context),
      readersCount_ (0),
      anyWriter_ (false)
{

}

ReadLock &ReadWriteGuard::Read ()
{
    return read_;
}

WriteLock &ReadWriteGuard::Write ()
{
    return write_;
}


}