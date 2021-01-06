#include <boost/test/unit_test.hpp>
#include <boost/format.hpp>

#include <cmath>
#include <thread>
#include <future>

#include <Miami/Disco/Disco.hpp>

#include "Utils.hpp"

BOOST_AUTO_TEST_SUITE (Context)

#define TEST_WORKERS_COUNT 6

BOOST_AUTO_TEST_CASE (StartupAndShutdown)
{
    auto *context = new Miami::Disco::Context (TEST_WORKERS_COUNT);
    delete context;
}

BOOST_AUTO_TEST_CASE (KernelModeFromExternal)
{
    Miami::Disco::Context context {TEST_WORKERS_COUNT};
    MutexCheckData checkData;

    auto threadFunction = [&checkData, &context] (uint8_t filler)
    {
        auto kernelGuard = context.KernelMode ().Enter ();
        for (uint32_t index = 0; index < MutexCheckData::chunkSize; ++index)
        {
            checkData.storage.push_back (filler);
        }
    };

    std::thread first (threadFunction, MutexCheckData::firstFiller);
    std::thread second (threadFunction, MutexCheckData::secondFiller);

    first.join ();
    second.join ();
    checkData.DoCheck ();
}

BOOST_AUTO_TEST_CASE (KernelModeFromTask)
{
    Miami::Disco::Context context {TEST_WORKERS_COUNT};
    MutexCheckData checkData;

    std::function<void (uint8_t)> taskBase = [&context, &checkData] (uint8_t filler)
    {
        auto kernelGuard = context.KernelMode ().Enter ();
        for (uint32_t index = 0; index < MutexCheckData::chunkSize; ++index)
        {
            checkData.storage.push_back (filler);
        }
    };

    std::promise<void> firstFinished;
    std::promise<void> secondFinished;

    context.Tasks ().Push (FabricateTestableTask(taskBase, firstFinished, MutexCheckData::firstFiller));
    context.Tasks ().Push (FabricateTestableTask(taskBase, secondFinished, MutexCheckData::secondFiller));

    firstFinished.get_future().wait();
    secondFinished.get_future().wait();
    checkData.DoCheck ();
}

BOOST_AUTO_TEST_CASE (LotsOfSeparateTasks)
{
    Miami::Disco::Context context {TEST_WORKERS_COUNT};
    std::vector <uint64_t> results;
    std::vector <std::promise<void>> finishes;

    constexpr uint32_t runsCount = 10;
    constexpr uint64_t tasksCount = 8192;

    results.resize(tasksCount, 0);
    for (uint64_t taskIndex = 0; taskIndex < tasksCount; ++taskIndex)
    {
        finishes.emplace_back();
    }

#define PERMUTATION(input) static_cast<uint64_t>(sqrt(input))

    std::function<void (uint64_t)> taskBase = [&results] (uint64_t input)
    {
        for (uint32_t runIndex = 0; runIndex < runsCount; ++runIndex)
        {
            results[input] = PERMUTATION(input);
        }
    };

    for (uint64_t taskIndex = 0; taskIndex < tasksCount; ++taskIndex)
    {
        context.Tasks ().Push (FabricateTestableTask(taskBase, finishes[taskIndex], taskIndex));
    }

    for (uint64_t taskIndex = 0; taskIndex < tasksCount; ++taskIndex)
    {
        finishes[taskIndex].get_future().wait();
        BOOST_REQUIRE(results[taskIndex] == PERMUTATION(taskIndex));
    }

#undef PERMUTATION
}

BOOST_AUTO_TEST_SUITE_END ()