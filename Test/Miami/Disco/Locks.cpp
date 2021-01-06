#include <boost/test/unit_test.hpp>
#include <boost/format.hpp>

#include <chrono>
#include <thread>
#include <future>

#include <Miami/Disco/Disco.hpp>

#include "Utils.hpp"

BOOST_AUTO_TEST_SUITE (Locks)

#define TEST_WORKERS_COUNT 6

BOOST_AUTO_TEST_CASE (SimpleLockFromTask)
{
    Miami::Disco::Context context {TEST_WORKERS_COUNT};
    {
        Miami::Disco::Lock lock {&context};
        MutexCheckData checkData;

        std::function <void (uint8_t)> taskBase = [&checkData, &lock] (uint8_t filler)
        {
            while (!lock.TryLock ())
            {
            }

            for (uint32_t index = 0; index < MutexCheckData::chunkSize; ++index)
            {
                checkData.storage.push_back (filler);
            }

            lock.Unlock ();
        };

        std::promise <void> firstFinished;
        std::promise <void> secondFinished;

        context.Tasks ().Push (FabricateTestableTask (taskBase, firstFinished, MutexCheckData::firstFiller));
        context.Tasks ().Push (FabricateTestableTask (taskBase, secondFinished, MutexCheckData::secondFiller));

        firstFinished.get_future ().wait ();
        secondFinished.get_future ().wait ();
        checkData.DoCheck ();
    }
}

BOOST_AUTO_TEST_CASE (SimpleLockFromExternal)
{
    Miami::Disco::Context context {TEST_WORKERS_COUNT};
    {
        Miami::Disco::Lock lock {&context};
        MutexCheckData checkData;

        auto threadFunction = [&checkData, &lock] (uint8_t filler)
        {
            while (!lock.TryLock ())
            {
            }

            for (uint32_t index = 0; index < MutexCheckData::chunkSize; ++index)
            {
                checkData.storage.push_back (filler);
            }

            lock.Unlock ();
        };

        std::thread first (threadFunction, MutexCheckData::firstFiller);
        std::thread second (threadFunction, MutexCheckData::secondFiller);

        first.join ();
        second.join ();
        checkData.DoCheck ();
    }
}

BOOST_AUTO_TEST_CASE (MultipleReadAllowed)
{
    Miami::Disco::Context context {0};
    {
        Miami::Disco::ReadWriteGuard readWriteGuard {&context};
        BOOST_REQUIRE_MESSAGE(readWriteGuard.Read ().TryLock (), "First read lock.");
        BOOST_REQUIRE_MESSAGE(readWriteGuard.Read ().TryLock (), "Second read lock.");
    }
}

BOOST_AUTO_TEST_CASE (OneWriteAllowed)
{
    Miami::Disco::Context context {0};
    {
        Miami::Disco::ReadWriteGuard readWriteGuard {&context};
        BOOST_REQUIRE_MESSAGE(readWriteGuard.Write ().TryLock (), "First write lock.");
        BOOST_REQUIRE_MESSAGE(!readWriteGuard.Write ().TryLock (), "Second write lock.");
    }
}

BOOST_AUTO_TEST_CASE (NoWriteWhileReading)
{
    Miami::Disco::Context context {0};
    {
        Miami::Disco::ReadWriteGuard readWriteGuard {&context};
        BOOST_REQUIRE(readWriteGuard.Read ().TryLock ());
        BOOST_REQUIRE(!readWriteGuard.Write ().TryLock ());
    }
}

BOOST_AUTO_TEST_CASE (NoReadWhileWriting)
{
    Miami::Disco::Context context {0};
    {
        Miami::Disco::ReadWriteGuard readWriteGuard {&context};
        BOOST_REQUIRE(readWriteGuard.Write ().TryLock ());
        BOOST_REQUIRE(!readWriteGuard.Read ().TryLock ());
    }
}

BOOST_AUTO_TEST_CASE (ReadWriteGuardUnlocks)
{
    Miami::Disco::Context context {0};
    {
        Miami::Disco::ReadWriteGuard readWriteGuard {&context};
        BOOST_REQUIRE_MESSAGE(readWriteGuard.Write ().TryLock (), "First write lock attempt.");
        readWriteGuard.Write ().Unlock ();

        BOOST_REQUIRE_MESSAGE(readWriteGuard.Write ().TryLock (), "Second write lock attempt.");
        readWriteGuard.Write ().Unlock ();

        BOOST_REQUIRE_MESSAGE(readWriteGuard.Read ().TryLock (), "First read lock.");
        BOOST_REQUIRE_MESSAGE(readWriteGuard.Read ().TryLock (), "Second read lock.");
        readWriteGuard.Read ().Unlock ();

        BOOST_REQUIRE_MESSAGE(!readWriteGuard.Write ().TryLock (),
                              "Third write lock attempt, reading is not finished yet.");

        readWriteGuard.Read ().Unlock ();
        BOOST_REQUIRE_MESSAGE(readWriteGuard.Write ().TryLock (), "Fourth write lock attempt.");
        readWriteGuard.Write ().Unlock ();
    }
}

BOOST_AUTO_TEST_CASE (ReadWriteGuardMultithreaded)
{
    Miami::Disco::Context context {TEST_WORKERS_COUNT};
    {
        srand (time (nullptr));
        using namespace std::chrono_literals;
        using Clock = std::chrono::high_resolution_clock;
        using TimePoint = std::chrono::time_point <Clock>;
        using Interval = std::pair <TimePoint, TimePoint>;

        constexpr std::chrono::milliseconds waitTime = 10ms;
        constexpr uint32_t writersCount = 16;
        constexpr uint32_t readersCount = 32;
        constexpr uint32_t tasksCount = writersCount + readersCount;

        Miami::Disco::ReadWriteGuard readWriteGuard {&context};
        std::function <void (Interval *)> baseReader = [&readWriteGuard, &waitTime] (Interval *output)
        {
            while (!readWriteGuard.Read ().TryLock ())
            {
            }

            output->first = Clock::now ();
            std::this_thread::sleep_for (waitTime);
            output->second = Clock::now ();
            readWriteGuard.Read ().Unlock ();

            // Add random wait to cover more cases.
            std::this_thread::sleep_for (std::chrono::milliseconds (1 + rand () % 5));
        };

        std::function <void (Interval *)> baseWriter = [&readWriteGuard, &waitTime] (Interval *output)
        {
            while (!readWriteGuard.Write ().TryLock ())
            {
            }

            output->first = Clock::now ();
            std::this_thread::sleep_for (waitTime);
            output->second = Clock::now ();
            readWriteGuard.Write ().Unlock ();

            // Add random wait to cover more cases.
            std::this_thread::sleep_for (std::chrono::milliseconds (1 + rand () % 5));
        };

        std::vector <Interval> readersIntervals (readersCount, {Clock::now (), Clock::now ()});
        std::vector <Interval> writersIntervals (writersCount, {Clock::now (), Clock::now ()});
        std::vector <std::promise <void>> finishes;

        for (uint32_t taskIndex = 0; taskIndex < tasksCount; ++taskIndex)
        {
            finishes.emplace_back ();
        }

        for (uint32_t readerIndex = 0; readerIndex < readersCount; ++readerIndex)
        {
            context.Tasks ().Push (FabricateTestableTask (
                baseReader, finishes[readerIndex], &readersIntervals[readerIndex]));
        }

        for (uint32_t writerIndex = 0; writerIndex < writersCount; ++writerIndex)
        {
            context.Tasks ().Push (FabricateTestableTask (
                baseWriter, finishes[readersCount + writerIndex], &writersIntervals[writerIndex]));
        }

        for (uint32_t taskIndex = 0; taskIndex < tasksCount; ++taskIndex)
        {
            finishes[taskIndex].get_future ().wait ();
        }

        auto NotIntersect = [] (const Interval &first, const Interval &second)
        {
            return first.second <= second.first || first.first >= second.second;
        };

        for (uint32_t writerIndex = 0; writerIndex < writersCount; ++writerIndex)
        {
            const Interval &first = writersIntervals[writerIndex];
            for (uint32_t readerIndex = 0; readerIndex < readersCount; ++readerIndex)
            {
                const Interval &second = readersIntervals[readerIndex];
                BOOST_REQUIRE_MESSAGE(NotIntersect (first, second),
                                      boost::format (
                                          "Found intersection between intervals (%1%, %2%) and (%3%, %4%)!") %
                                      first.first.time_since_epoch().count() % first.second.time_since_epoch().count() %
                                      second.first.time_since_epoch().count() % second.second.time_since_epoch().count());
            }

            for (uint32_t anotherWriterIndex = 0; anotherWriterIndex < writersCount; ++anotherWriterIndex)
            {
                if (writerIndex != anotherWriterIndex)
                {
                    const Interval &second = readersIntervals[anotherWriterIndex];
                    BOOST_REQUIRE_MESSAGE(NotIntersect (first, second),
                                          boost::format (
                                              "Found intersection between intervals (%1%, %2%) and (%3%, %4%)!") %
                                          first.first.time_since_epoch().count() % first.second.time_since_epoch().count() %
                                          second.first.time_since_epoch().count() % second.second.time_since_epoch().count());
                }
            }
        }
    }
}

BOOST_AUTO_TEST_SUITE_END ()