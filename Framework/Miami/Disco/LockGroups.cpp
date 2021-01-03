#include <cassert>

#include <Miami/Disco/LockGroups.hpp>

namespace Miami::Disco
{
namespace LockGroupsDetails
{
template <typename Argument>
static void Deploy (Context *context, std::function <void (Argument)> &task, Argument argument,
                    KernelModeGuard::RAII &kernelModeGuard)
{
    assert(kernelModeGuard.IsValid ());
    kernelModeGuard.AfterExit (
        [context, taskLambda = std::move (task), taskArgument = std::move (argument)] () mutable
        {
            context->Tasks ().Push (
                [callee = std::move (taskLambda), calleeArgument = std::move (taskArgument)] () mutable
                {
                    callee (std::move (calleeArgument));
                });
        });
}

static void Invalidate (Context *context, std::function <void ()> &cancel,
                        KernelModeGuard::RAII &kernelModeGuard)
{
    assert(kernelModeGuard.IsValid ());
    if (context)
    {
        kernelModeGuard.AfterExit ([context, cancelLambda = std::move (cancel)] () mutable
                                   {
                                       context->Tasks ().Push (std::move (cancelLambda));
                                   });
    }
}
}

OneLockGroup::OneLockGroup (const AnyLockPointer &lock, OneLockGroup::NextLambda next,
                            OneLockGroup::CancelLambda cancel, KernelModeGuard::RAII &kernelModeGuard)
    : lock_ (lock),
      next_ (std::move (next)),
      cancel_ (std::move (cancel))
{
    assert(kernelModeGuard.IsValid ());
    lock_.RegisterLockGroup (AnyLockGroupPointer (this), kernelModeGuard);
}

OneLockGroup::~OneLockGroup ()
{
    if (lock_.GetContext ())
    {
        KernelModeGuard::RAII guard = lock_.GetContext ()->KernelMode ().Enter ();
        lock_.UnregisterLockGroup (AnyLockGroupPointer (this), guard);
    }
}

bool OneLockGroup::TryCapture (const AnyLockPointer &lock, OneLockGroup::NextLambda &next,
                               KernelModeGuard::RAII &kernelModeGuard)
{
    assert(kernelModeGuard.IsValid ());
    Context *context = lock.GetContext ();

    if (context && lock.TryLock (kernelModeGuard))
    {
        std::shared_ptr <SafeLockGuard> lockGuard {new SafeLockGuard (lock, kernelModeGuard)};
        LockGroupsDetails::Deploy (context, next, std::move (lockGuard), kernelModeGuard);
        return true;
    }
    else
    {
        return false;
    }
}

bool OneLockGroup::TryCapture (KernelModeGuard::RAII &kernelModeGuard)
{
    assert(kernelModeGuard.IsValid ());
    if (TryCapture (lock_, next_, kernelModeGuard))
    {
        lock_.Nullify ();
        return true;
    }
    else
    {
        return false;
    }
}

void OneLockGroup::Invalidate (void *invalidationSourceLock, KernelModeGuard::RAII &kernelModeGuard)
{
    assert(kernelModeGuard.IsValid ());
    assert(lock_.Is (invalidationSourceLock));

    LockGroupsDetails::Invalidate (lock_.GetContext (), cancel_, kernelModeGuard);
    lock_.Nullify ();
    delete this;
}

MultipleLockGroup::MultipleLockGroup (std::vector <AnyLockPointer> locks, MultipleLockGroup::NextLambda next,
                                      MultipleLockGroup::CancelLambda cancel, KernelModeGuard::RAII &kernelModeGuard)
    : locks_ (std::move (locks)),
      next_ (std::move (next)),
      cancel_ (std::move (cancel))
{
    assert(kernelModeGuard.IsValid ());
    assert(!locks.empty ());

    for (AnyLockPointer &lock : locks_)
    {
        assert(!lock.IsNull ());
        assert(lock.GetContext () == locks_[0].GetContext ());
        lock.RegisterLockGroup (AnyLockGroupPointer (this), kernelModeGuard);
    }
}

MultipleLockGroup::~MultipleLockGroup ()
{
    if (!locks_.empty ())
    {
        // In debug mode, check that either all locks are valid or all locks are invalid.
        assert((locks_[0].IsNull () &&
                std::find_if (locks_.begin (), locks_.end (),
                              [] (AnyLockPointer &lock)
                              {
                                  return !lock.IsNull ();
                              }) == locks_.end ()) ||
               (!locks_[0].IsNull () &&
                std::find_if (locks_.begin (), locks_.end (),
                              [] (AnyLockPointer &lock)
                              {
                                  return lock.IsNull ();
                              }) == locks_.end ()));

        if (locks_[0].GetContext ())
        {
            KernelModeGuard::RAII guard = locks_[0].GetContext ()->KernelMode ().Enter ();
            for (AnyLockPointer &lock : locks_)
            {
                lock.UnregisterLockGroup (AnyLockGroupPointer (this), guard);
            }
        }
    }
}

bool MultipleLockGroup::TryCapture (const std::vector <AnyLockPointer> &locks, MultipleLockGroup::NextLambda &next,
                                    KernelModeGuard::RAII &kernelModeGuard)
{
    assert(kernelModeGuard.IsValid ());
    assert(!locks.empty ());

    if (locks.empty ())
    {
        return false;
    }

    Context *context = locks[0].GetContext ();
    if (context && TryLockAll (locks, kernelModeGuard))
    {
        std::vector <std::shared_ptr <SafeLockGuard>> guards;
        guards.reserve (locks.size ());

        for (const AnyLockPointer &lock : locks)
        {
            guards.emplace_back (std::shared_ptr <SafeLockGuard> (new SafeLockGuard (lock, kernelModeGuard)));
        }

        LockGroupsDetails::Deploy (context, next, std::move (guards), kernelModeGuard);
        return true;
    }
    else
    {
        return false;
    }
}

bool MultipleLockGroup::TryCapture (KernelModeGuard::RAII &kernelModeGuard)
{
    assert(kernelModeGuard.IsValid ());
    if (TryCapture (locks_, next_, kernelModeGuard))
    {
        for (AnyLockPointer &lock : locks_)
        {
            lock.Nullify ();
        }

        return true;
    }
    else
    {
        return false;
    }
}

void MultipleLockGroup::Invalidate (void *invalidationSourceLock, KernelModeGuard::RAII &kernelModeGuard)
{
    assert(kernelModeGuard.IsValid ());
    assert(std::find_if (locks_.begin (), locks_.end (),
                         [invalidationSourceLock] (const AnyLockPointer &pointer)
                         {
                             return pointer.Is (invalidationSourceLock);
                         }) != locks_.end ());

    if (!locks_.empty ())
    {
        LockGroupsDetails::Invalidate (locks_[0].GetContext (), cancel_, kernelModeGuard);
    }

    for (AnyLockPointer &lock : locks_)
    {
        lock.Nullify ();
    }

    delete this;
}
}