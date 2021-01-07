#include <cassert>

#include <Miami/Disco/Context.hpp>

namespace Miami::Disco
{
KernelModeGuard::RAII::RAII (KernelModeGuard::RAII &&another) noexcept
    : owner_ (another.owner_),
      afterExit_ (std::move (another.afterExit_))
{
    another.owner_ = nullptr;
}

KernelModeGuard::RAII::RAII (KernelModeGuard *owner) noexcept
    : owner_ (owner)
{
}

KernelModeGuard::RAII::~RAII ()
{
    if (owner_)
    {
        owner_->Exit ();
        for (const std::function <void ()> &callback : afterExit_)
        {
            callback ();
        }
    }
}

bool KernelModeGuard::RAII::IsValid () const
{
    return owner_;
}

void KernelModeGuard::RAII::AfterExit (std::function <void ()> callback)
{
    afterExit_.emplace_back (std::move (callback));
}

KernelModeGuard::RAII KernelModeGuard::Enter ()
{
    guard_.lock ();
    return KernelModeGuard::RAII {this};
}

void KernelModeGuard::Exit ()
{
    guard_.unlock ();
}

TaskPool::TaskPool (uint32_t capacity)
    : guard_ (),
      tasksAvailable_ (),
      spaceAvailable_ (),
      begin_ (0u),
      end_ (0u),
      capacity_ (capacity),
      content_ (new std::function <void ()>[capacity])
{

}

TaskPool::~TaskPool ()
{
    delete[] content_;
}

void TaskPool::Push (std::function <void ()> task)
{
    std::unique_lock <std::mutex> lock (guard_);
    spaceAvailable_.wait (lock,
                          [this]
                          {
                              return IncreaseIndex (end_) != begin_;
                          });

    content_[end_] = std::move (task);
    end_ = IncreaseIndex (end_);
    lock.unlock ();
    tasksAvailable_.notify_one ();
}

std::function <void ()> TaskPool::Pop (const std::chrono::milliseconds &timeout)
{
    std::unique_lock <std::mutex> lock (guard_);
    if (!tasksAvailable_.wait_for (lock, timeout,
                                   [this]
                                   {
                                       return begin_ != end_;
                                   }))
    {
        return std::function <void ()> {};
    }

    std::function <void ()> task = std::move (content_[begin_]);
    begin_ = IncreaseIndex (begin_);
    lock.unlock ();
    spaceAvailable_.notify_one ();
    return task;
}

uint32_t TaskPool::IncreaseIndex (uint32_t index) const
{
    return (index + 1) % capacity_;
}

Context::Context (uint32_t workerThreads)
    : workers_ (),
      taskPool_ (workerThreads),
      kernelModeGuard_ (),
      isShuttingDown_ (false)
{
    auto threadFunction = [this]
    {
        while (!IsShuttingDown ())
        {
            using namespace std::chrono_literals;
            std::function <void ()> task = taskPool_.Pop (100ms);

            if (task)
            {
                task ();
            }
        }
    };

    for (uint32_t workerIndex = 0; workerIndex < workerThreads; ++workerIndex)
    {
        workers_.emplace_back (threadFunction);
    }
}

Context::~Context ()
{
    isShuttingDown_ = true;
    for (std::thread &worker : workers_)
    {
        if (worker.joinable ())
        {
            worker.join ();
        }
        else
        {
            assert(false);
        }
    }
}

TaskPool &Context::Tasks ()
{
    return taskPool_;
}

KernelModeGuard &Context::KernelMode ()
{
    return kernelModeGuard_;
}

bool Context::IsShuttingDown () const
{
    return isShuttingDown_;
}
}
