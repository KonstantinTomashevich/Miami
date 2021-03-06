#include <cassert>
#include <Miami/Disco/Disco.hpp>

namespace Miami::Disco
{
void After (const AnyLockPointer &lock, OneLockGroup::NextLambda next, OneLockGroup::CancelLambda cancel)
{
    assert(!lock.IsNull ());
    Context *context = lock.GetContext ();
    assert(context);

    if (context)
    {
        KernelModeGuard::RAII guard = lock.GetContext ()->KernelMode ().Enter ();
        if (!OneLockGroup::TryCapture (lock, next, guard))
        {
            new OneLockGroup (lock, std::move (next), std::move (cancel), guard);
        }
    }
}

void After (const std::vector <AnyLockPointer> &locks, MultipleLockGroup::NextLambda next,
            MultipleLockGroup::CancelLambda cancel)
{
    // TODO: It'll be good to check for conflicts inside given locks array: duplicates and read-write pairs.
    assert(locks.size () > 1);
    assert(std::find_if (locks.begin (), locks.end (),
                         [] (const AnyLockPointer &lock)
                         {
                             return lock.IsNull ();
                         }) == locks.end ());

    if (!locks.empty ())
    {
        Context *context = locks[0].GetContext ();
        assert(context);

        if (context)
        {
            KernelModeGuard::RAII guard = locks[0].GetContext ()->KernelMode ().Enter ();
            if (!MultipleLockGroup::TryCapture (locks, next, guard))
            {
                new MultipleLockGroup (locks, std::move (next), std::move (cancel), guard);
            }
        }
    }
}

bool IsReadOrWriteCaptured (
    const std::shared_ptr <Disco::SafeLockGuard> &readOrWriteGuard, const ReadWriteGuard &guard)
{
    return (readOrWriteGuard != nullptr && readOrWriteGuard->Is (&guard.Read ())) ||
           IsWriteCaptured (readOrWriteGuard, guard);
}

bool IsWriteCaptured (const std::shared_ptr <Disco::SafeLockGuard> &writeGuard, const ReadWriteGuard &guard)
{
    return writeGuard != nullptr && writeGuard->Is (&guard.Write ());
}
}