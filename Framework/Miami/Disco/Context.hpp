#pragma once

#include <cstdint>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <functional>
#include <vector>
#include <thread>

#include <Miami/Disco/Annotations.hpp>
#include <Miami/Annotations.hpp>

namespace Miami::Disco
{
class KernelModeGuard final
{
public:
    class RAII final
    {
    public:
        explicit RAII (const RAII &another) = delete;

        RAII (RAII &&another) noexcept;

        ~RAII ();

        bool IsValid () const;

        void AfterExit (std::function <void ()> callback);

        RAII &operator = (const RAII &another) = delete;

    private:
        explicit RAII (KernelModeGuard *owner) noexcept;

        KernelModeGuard *owner_;
        std::vector <std::function <void ()>> afterExit_;

        friend class KernelModeGuard;
    };

    KernelModeGuard () = default;

    RAII Enter ();

    KernelModeGuard &operator = (const KernelModeGuard &another) = delete;

private:
    void Exit ();

    std::mutex guard_;

    friend class RAII;
};

class TaskPool final
{
public:
    explicit TaskPool (uint32_t capacity);

    ~TaskPool ();

    void Push (std::function <void ()> task);

    std::function <void ()> Pop (const std::chrono::milliseconds &timeout);

private:
    uint32_t IncreaseIndex (uint32_t index) const;

    std::mutex guard_;
    std::condition_variable tasksAvailable_;
    std::condition_variable spaceAvailable_;

    uint32_t begin_;
    uint32_t end_;
    uint32_t capacity_;
    std::function <void ()> *content_;
};

// TODO: Multithreaded logging (maybe as separate Evan library).
class Context final
{
public:
    explicit Context (uint32_t workerThreads);

    ~Context ();

    free_call TaskPool &Tasks ();

    free_call KernelModeGuard &KernelMode ();

    free_call bool IsShuttingDown () const;

private:
    std::vector <std::thread> workers_;
    TaskPool taskPool_;
    KernelModeGuard kernelModeGuard_;
    bool isShuttingDown_;
};
}
