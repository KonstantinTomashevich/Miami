#pragma once

#include <cstdint>
#include <vector>
#include <functional>

#include <Miami/Disco/Annotations.hpp>
#include <Miami/Disco/Context.hpp>
#include <Miami/Disco/Variants.hpp>

namespace Miami::Disco
{
// TODO: Rename lock groups to await groups? Sounds better, I think.
// TODO: TimedOneLockGroup. Also, cancel lambda for timed lock group must contain cancel reason.
class OneLockGroup
{
public:
    static constexpr LockGroupType LOCK_GROUP_TYPE = LockGroupType::ONE;

    // TODO: We can not use unique_ptr, because std::function lambda must be copyable.
    //       In future, try to replace with fu2::unique_function and return unique_ptr back.
    //       (fu2::unique_function is from https://github.com/Naios/function2).
    using NextLambda = std::function <void (std::shared_ptr <SafeLockGuard>)>;
    using CancelLambda = std::function <void ()>;

    // TODO: Construction must be done only if given lock can not be locked right now.
    OneLockGroup (const AnyLockPointer &lock, NextLambda next,
                  CancelLambda cancel, KernelModeGuard::RAII &kernelModeGuard);

    ~OneLockGroup () = default;

    static bool TryCapture (const AnyLockPointer &lock, NextLambda &next, KernelModeGuard::RAII &kernelModeGuard);

private:
    bool TryCapture (void *captureSourceLock, KernelModeGuard::RAII &kernelModeGuard);

    void Invalidate (void *invalidationSourceLock, KernelModeGuard::RAII &kernelModeGuard);

    void Destruct (KernelModeGuard::RAII &kernelModeGuard);

    AnyLockPointer lock_;
    NextLambda next_;
    CancelLambda cancel_;

    friend class AnyLockGroupPointer;
};

// TODO: TimedMultipleLockGroup. Also, cancel lambda for timed multiple lock group must contain cancel reason.
class MultipleLockGroup
{
public:
    static constexpr LockGroupType LOCK_GROUP_TYPE = LockGroupType::MULTIPLE;

    // TODO: See remark about pointer type in OneLockGroup::NextLambda;
    using NextLambda = std::function <void (std::vector <std::shared_ptr <SafeLockGuard>>)>;

    // TODO: Should contain cancel reason (for example, lock X destruction).
    using CancelLambda = std::function <void ()>;

    MultipleLockGroup (std::vector <AnyLockPointer> locks, NextLambda next,
                       CancelLambda cancel, KernelModeGuard::RAII &kernelModeGuard);

    ~MultipleLockGroup () = default;

    static bool TryCapture (const std::vector <AnyLockPointer> &locks, NextLambda &next,
                            KernelModeGuard::RAII &kernelModeGuard);

private:
    bool TryCapture (void *captureSourceLock, KernelModeGuard::RAII &kernelModeGuard);

    void Invalidate (void *invalidationSourceLock, KernelModeGuard::RAII &kernelModeGuard);

    void Destruct (void *skipLock, KernelModeGuard::RAII &kernelModeGuard);

    std::vector <AnyLockPointer> locks_;
    NextLambda next_;
    CancelLambda cancel_;

    friend class AnyLockGroupPointer;
};
}
