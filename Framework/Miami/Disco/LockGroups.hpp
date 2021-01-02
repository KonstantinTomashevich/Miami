#pragma once

#include <cstdint>
#include <vector>
#include <functional>

#include <Miami/Disco/Annotations.hpp>
#include <Miami/Disco/Context.hpp>
#include <Miami/Disco/Variants.hpp>

namespace Miami::Disco
{
// TODO: TimedOneLockGroup.
class OneLockGroup
{
public:
    using NextLambda = std::function <void (std::unique_ptr <SafeLockGuard>)>;
    using CancelLambda = std::function <void ()>;

    // TODO: Construction must be done only if given lock can not be locked right now.
    OneLockGroup (const AnyLockPointer &lock, NextLambda next,
                  CancelLambda cancel, KernelModeGuard::RAII &kernelModeGuard);

    kernel_call ~OneLockGroup ();

    bool TryCapture (KernelModeGuard::RAII &kernelModeGuard);

    void Invalidate (KernelModeGuard::RAII &kernelModeGuard);

private:
    static bool TryCapture (AnyLockPointer &lock, NextLambda &next, KernelModeGuard::RAII &kernelModeGuard);

    AnyLockPointer lock_;
    NextLambda next_;
    CancelLambda cancel_;
};

// TODO: TimedMultipleLockGroup.
class MultipleLockGroup
{

};
}
