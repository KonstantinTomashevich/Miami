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
        MutexCheckCommons checkData;

        std::function <void (uint8_t)> taskBase = [&checkData, &lock] (uint8_t filler)
        {
            while (!lock.TryLock ())
            {
            }

            for (uint32_t index = 0; index < MutexCheckCommons::chunkSize; ++index)
            {
                checkData.storage.push_back (filler);
            }

            lock.Unlock ();
        };

        std::promise <void> firstFinished;
        std::promise <void> secondFinished;

        context.Tasks ().Push (FabricateTestableTask (taskBase, firstFinished, MutexCheckCommons::firstFiller));
        context.Tasks ().Push (FabricateTestableTask (taskBase, secondFinished, MutexCheckCommons::secondFiller));

        firstFinished.get_future ().wait ();
        secondFinished.get_future ().wait ();
        checkData.CheckResults ();
    }
}

BOOST_AUTO_TEST_CASE (SimpleLockFromExternal)
{
    Miami::Disco::Context context {TEST_WORKERS_COUNT};
    {
        Miami::Disco::Lock lock {&context};
        MutexCheckCommons checkData;

        auto threadFunction = [&checkData, &lock] (uint8_t filler)
        {
            while (!lock.TryLock ())
            {
            }

            for (uint32_t index = 0; index < MutexCheckCommons::chunkSize; ++index)
            {
                checkData.storage.push_back (filler);
            }

            lock.Unlock ();
        };

        std::thread first (threadFunction, MutexCheckCommons::firstFiller);
        std::thread second (threadFunction, MutexCheckCommons::secondFiller);

        first.join ();
        second.join ();
        checkData.CheckResults ();
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
        constexpr std::chrono::milliseconds waitTime = 10ms;

        ReaderWriterSwarmCheckCommons swarmCheck;
        Miami::Disco::ReadWriteGuard readWriteGuard {&context};

        std::function <void (ReaderWriterSwarmCheckCommons::Interval * )>
            baseReader = [&readWriteGuard, &waitTime] (ReaderWriterSwarmCheckCommons::Interval *output)
        {
            while (!readWriteGuard.Read ().TryLock ())
            {
            }

            output->first = ReaderWriterSwarmCheckCommons::Clock::now ();
            std::this_thread::sleep_for (waitTime);
            output->second = ReaderWriterSwarmCheckCommons::Clock::now ();
            readWriteGuard.Read ().Unlock ();

            // Add random wait to cover more cases.
            std::this_thread::sleep_for (std::chrono::milliseconds (1 + rand () % 5));
        };

        std::function <void (ReaderWriterSwarmCheckCommons::Interval * )>
            baseWriter = [&readWriteGuard, &waitTime] (ReaderWriterSwarmCheckCommons::Interval *output)
        {
            while (!readWriteGuard.Write ().TryLock ())
            {
            }

            output->first = ReaderWriterSwarmCheckCommons::Clock::now ();
            std::this_thread::sleep_for (waitTime);
            output->second = ReaderWriterSwarmCheckCommons::Clock::now ();
            readWriteGuard.Write ().Unlock ();

            // Add random wait to cover more cases.
            std::this_thread::sleep_for (std::chrono::milliseconds (1 + rand () % 5));
        };

        std::vector <std::promise <void>> finishes;
        for (uint32_t taskIndex = 0; taskIndex < ReaderWriterSwarmCheckCommons::tasksCount; ++taskIndex)
        {
            finishes.emplace_back ();
        }

        for (uint32_t readerIndex = 0; readerIndex < ReaderWriterSwarmCheckCommons::readersCount; ++readerIndex)
        {
            context.Tasks ().Push (FabricateTestableTask (
                baseReader, finishes[readerIndex], &swarmCheck.readersIntervals[readerIndex]));
        }

        for (uint32_t writerIndex = 0; writerIndex < ReaderWriterSwarmCheckCommons::writersCount; ++writerIndex)
        {
            context.Tasks ().Push (FabricateTestableTask (
                baseWriter, finishes[ReaderWriterSwarmCheckCommons::readersCount + writerIndex],
                &swarmCheck.writersIntervals[writerIndex]));
        }

        for (uint32_t taskIndex = 0; taskIndex < ReaderWriterSwarmCheckCommons::tasksCount; ++taskIndex)
        {
            finishes[taskIndex].get_future ().wait ();
        }

        swarmCheck.CheckResults ();
    }
}

BOOST_AUTO_TEST_SUITE_END ()