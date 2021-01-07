#pragma once

#include <cstdint>
#include <vector>
#include <functional>
#include <future>

class MutexCheckCommons final
{
public:
    static constexpr uint32_t chunkSize = 1024;
    static constexpr uint8_t firstFiller = 1;
    static constexpr uint8_t secondFiller = 2;

    std::vector <uint8_t> storage;

    void CheckResults () const;
};

class ReaderWriterSwarmCheckCommons
{
public:
    using Clock = std::chrono::high_resolution_clock;
    using TimePoint = std::chrono::time_point <Clock>;
    using Interval = std::pair <TimePoint, TimePoint>;

    static constexpr uint32_t writersCount = 16;
    static constexpr uint32_t readersCount = 32;
    static constexpr uint32_t tasksCount = writersCount + readersCount;

    std::vector <Interval> readersIntervals {readersCount, {Clock::now (), Clock::now ()}};
    std::vector <Interval> writersIntervals {writersCount, {Clock::now (), Clock::now ()}};

    void CheckResults () const;
};

template <typename... Args>
std::function <void ()>
FabricateTestableTask (const std::function <void (Args...)> &base, std::promise <void> &finishPromise, Args... args)
{
    return [args..., &finishPromise, &base]
    {
        base (args...);
        finishPromise.set_value ();
    };
}