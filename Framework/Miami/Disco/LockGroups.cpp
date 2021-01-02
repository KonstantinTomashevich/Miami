#include <cassert>

#include <Miami/Disco/LockGroups.hpp>

namespace Miami::Disco
{
OneLockGroup::OneLockGroup (const AnyLockPointer &lock, OneLockGroup::NextLambda next,
                            OneLockGroup::CancelLambda cancel, KernelModeGuard::RAII &kernelModeGuard)
    : lock_ (lock),
      next_ (std::move (next)),
      cancel_ (std::move (cancel))
{
    lock_.RegisterLockGroup (AnyLockGroupPointer < LockGroupType::ONE > (this), kernelModeGuard);
}

OneLockGroup::~OneLockGroup ()
{
    if (lock_.GetContext ())
    {
        KernelModeGuard::RAII guard = lock_.GetContext ()->KernelMode ().Enter ();
        lock_.UnregisterLockGroup (AnyLockGroupPointer < LockGroupType::ONE > (this), guard)
    }
}

bool OneLockGroup::TryCapture (KernelModeGuard::RAII &kernelModeGuard)
{
    assert(kernelModeGuard.IsValid ());
    return TryCapture (lock_, next_, kernelModeGuard);
}

void OneLockGroup::Invalidate (KernelModeGuard::RAII &kernelModeGuard)
{
    assert(kernelModeGuard.IsValid ());
    Context *context = lock_.GetContext ();

    if (context)
    {
        kernelModeGuard.AfterExit ([context, cancelLambda = std::move (cancel_)] () mutable
                                   {
                                       context->Tasks ().Push (cancelLambda);
                                   });
    }

    lock_.Nullify ();
    delete this;
}

bool OneLockGroup::TryCapture (AnyLockPointer &lock, OneLockGroup::NextLambda &next,
                               KernelModeGuard::RAII &kernelModeGuard)
{
    assert(kernelModeGuard.IsValid ());
    Context *context = lock.GetContext ();

    if (context && lock.TryLock (kernelModeGuard))
    {
        std::unique_ptr <SafeLockGuard> lockGuard {new SafeLockGuard (lock, kernelModeGuard)};
        lock.Nullify ();

        kernelModeGuard.AfterExit (
            [context, nextLambda = std::move (next), safeLockGuard = std::move (lockGuard)] () mutable
            {
                context->Tasks ().Push (
                    [callee = std::move (nextLambda), argument = std::move (safeLockGuard)] () mutable
                    {
                        callee (std::move (argument));
                    });
            });
    }
    else
    {
        return false;
    }
}
}