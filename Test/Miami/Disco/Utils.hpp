#pragma once

#include <cstdint>
#include <vector>
#include <functional>
#include <future>

class MutexCheckData final
{
public:
    static constexpr uint32_t chunkSize = 1024;
    static constexpr uint8_t firstFiller = 1;
    static constexpr uint8_t secondFiller = 2;

    std::vector <uint8_t> storage;

    void DoCheck () const;
};

template <typename... Args>
std::function <void ()>
FabricateTestableTask (const std::function <void (Args...)> &base, std::promise <void> &finishPromise, Args... args)
{
    return [args..., &finishPromise, &base]
    {
        base (args...);
        finishPromise.set_value();
    };
}