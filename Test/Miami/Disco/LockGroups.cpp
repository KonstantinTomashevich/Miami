#include <boost/test/unit_test.hpp>
#include <boost/format.hpp>

#include <chrono>
#include <thread>
#include <future>

#include <Miami/Disco/Disco.hpp>

#include "Utils.hpp"

BOOST_AUTO_TEST_SUITE (LockGroups)

#define TEST_WORKERS_COUNT 6

template <typename... Args>
std::function <void (std::shared_ptr <Miami::Disco::SafeLockGuard>)>
FabricateAfterOneTestableCallback (
    const std::function <void (std::shared_ptr <Miami::Disco::SafeLockGuard>, Args...)> &base,
    std::promise <void> &finishPromise, Args... args)
{
    return [args..., &finishPromise, &base] (std::shared_ptr <Miami::Disco::SafeLockGuard> guard)
    {
        base (std::move (guard), args...);
        finishPromise.set_value ();
    };
}

template <typename... Args>
std::function <void (std::vector <std::shared_ptr <Miami::Disco::SafeLockGuard>>)>
FabricateAfterMultipleTestableCallback (
    const std::function <void (std::vector <std::shared_ptr <Miami::Disco::SafeLockGuard>>, Args...)> &base,
    std::promise <void> &finishPromise, Args... args)
{
    return [args..., &finishPromise, &base] (std::vector <std::shared_ptr <Miami::Disco::SafeLockGuard>> guards)
    {
        base (std::move (guards), args...);
        finishPromise.set_value ();
    };
}

BOOST_AUTO_TEST_CASE (AfterSimpleLockMutualExclusion)
{
    Miami::Disco::Context context {TEST_WORKERS_COUNT};
    {
        Miami::Disco::Lock lock {&context};
        MutexCheckCommons checkData;

        std::function < void (std::shared_ptr <Miami::Disco::SafeLockGuard>, uint8_t) > taskBase =
            [&checkData] (const std::shared_ptr <Miami::Disco::SafeLockGuard> &guard, uint8_t filler)
            {
                for (uint32_t index = 0; index < MutexCheckCommons::chunkSize; ++index)
                {
                    checkData.storage.push_back (filler);
                }
            };

        std::promise <void> firstFinished;
        std::promise <void> secondFinished;

        Miami::Disco::After (
            Miami::Disco::AnyLockPointer (&lock),
            FabricateAfterOneTestableCallback (taskBase, firstFinished, MutexCheckCommons::firstFiller));

        Miami::Disco::After (
            Miami::Disco::AnyLockPointer (&lock),
            FabricateAfterOneTestableCallback (taskBase, secondFinished, MutexCheckCommons::secondFiller));

        firstFinished.get_future ().wait ();
        secondFinished.get_future ().wait ();
        checkData.CheckResults ();
    }
}

BOOST_AUTO_TEST_CASE (AfterSimpleLockCancel)
{
    Miami::Disco::Context context {TEST_WORKERS_COUNT};
    {
        auto *lock = new Miami::Disco::Lock {&context};
        BOOST_REQUIRE_MESSAGE(lock->TryLock (), "Lock must be captured after creation.");

        bool canceled = false;
        std::promise <void> finished;

        std::function < void (std::shared_ptr <Miami::Disco::SafeLockGuard>) > onNext =
            [&finished] (const std::shared_ptr <Miami::Disco::SafeLockGuard> &)
            {
                finished.set_value ();
            };


        std::function < void () > onCancel = [&canceled, &finished]
        {
            canceled = true;
            finished.set_value ();
        };

        Miami::Disco::After (Miami::Disco::AnyLockPointer (lock), onNext, onCancel);
        delete lock;
        finished.get_future ().wait ();
        BOOST_REQUIRE(canceled);
    }
}

BOOST_AUTO_TEST_CASE (AfterOneLockCaptureLockGuard)
{
    Miami::Disco::Context context {TEST_WORKERS_COUNT};
    {
        Miami::Disco::Lock lock {&context};
        std::shared_ptr <Miami::Disco::SafeLockGuard> capturedGuard = nullptr;
        std::promise <void> finished;

        std::function < void (std::shared_ptr <Miami::Disco::SafeLockGuard>) > onNext =
            [&capturedGuard, &finished] (std::shared_ptr <Miami::Disco::SafeLockGuard> guard)
            {
                capturedGuard = std::move (guard);
                finished.set_value ();
            };

        Miami::Disco::After (Miami::Disco::AnyLockPointer (&lock), onNext);
        finished.get_future ().wait ();
        BOOST_REQUIRE(capturedGuard);
        BOOST_REQUIRE_MESSAGE(!lock.TryLock (), "Lock should be captured even after task execution.");
    }
}

BOOST_AUTO_TEST_CASE (WriteAfterRead)
{
    Miami::Disco::Context context {TEST_WORKERS_COUNT};
    {
        Miami::Disco::ReadWriteGuard readWriteGuard {&context};
        using Clock = std::chrono::high_resolution_clock;
        using TimePoint = std::chrono::time_point <Clock>;

        std::promise <void> readAcquired;
        std::promise <void> readFinished;
        std::promise <void> writeFinished;

        TimePoint readPoint;
        TimePoint writePoint;

        std::function < void (std::shared_ptr <Miami::Disco::SafeLockGuard>) > doRead =
            [&readAcquired, &readFinished, &readPoint] (const std::shared_ptr <Miami::Disco::SafeLockGuard> &)
            {
                using namespace std::chrono_literals;
                readAcquired.set_value ();
                std::this_thread::sleep_for (5ms);
                readPoint = Clock::now ();
                readFinished.set_value ();
            };

        Miami::Disco::After (Miami::Disco::AnyLockPointer (&readWriteGuard.Read ()), doRead);
        readAcquired.get_future ().wait ();

        std::function < void (std::shared_ptr <Miami::Disco::SafeLockGuard>) > doWrite =
            [&writeFinished, &writePoint] (const std::shared_ptr <Miami::Disco::SafeLockGuard> &)
            {
                writePoint = Clock::now ();
                writeFinished.set_value ();
            };

        Miami::Disco::After (Miami::Disco::AnyLockPointer (&readWriteGuard.Write ()), doWrite);
        readFinished.get_future ().wait ();
        writeFinished.get_future ().wait ();
        BOOST_REQUIRE(readPoint <= writePoint);
    }
}

BOOST_AUTO_TEST_CASE (ReadAfterWrite)
{
    Miami::Disco::Context context {TEST_WORKERS_COUNT};
    {
        Miami::Disco::ReadWriteGuard readWriteGuard {&context};
        using Clock = std::chrono::high_resolution_clock;
        using TimePoint = std::chrono::time_point <Clock>;

        std::promise <void> writeAcquired;
        std::promise <void> writeFinished;
        std::promise <void> readFinished;

        TimePoint readPoint;
        TimePoint writePoint;

        std::function < void (std::shared_ptr <Miami::Disco::SafeLockGuard>) > doWrite =
            [&writeAcquired, &writeFinished, &writePoint] (const std::shared_ptr <Miami::Disco::SafeLockGuard> &)
            {
                using namespace std::chrono_literals;
                writeAcquired.set_value ();
                std::this_thread::sleep_for (5ms);
                writePoint = Clock::now ();
                writeFinished.set_value ();
            };

        Miami::Disco::After (Miami::Disco::AnyLockPointer (&readWriteGuard.Write ()), doWrite);
        writeAcquired.get_future ().wait ();

        std::function < void (std::shared_ptr <Miami::Disco::SafeLockGuard>) > doRead =
            [&readFinished, &readPoint] (const std::shared_ptr <Miami::Disco::SafeLockGuard> &)
            {
                readPoint = Clock::now ();
                readFinished.set_value ();
            };

        Miami::Disco::After (Miami::Disco::AnyLockPointer (&readWriteGuard.Read ()), doRead);
        readFinished.get_future ().wait ();
        writeFinished.get_future ().wait ();
        BOOST_REQUIRE(readPoint >= writePoint);
    }
}

BOOST_AUTO_TEST_CASE (AfterBasedReadWriteSwarm)
{
    Miami::Disco::Context context {TEST_WORKERS_COUNT};
    {
        srand (time (nullptr));
        using namespace std::chrono_literals;
        constexpr std::chrono::milliseconds waitTime = 10ms;

        ReaderWriterSwarmCheckCommons swarmCheck;
        Miami::Disco::ReadWriteGuard readWriteGuard {&context};

        std::function <void (std::shared_ptr <Miami::Disco::SafeLockGuard>, ReaderWriterSwarmCheckCommons::Interval *)>
            baseCallback = [&waitTime] (const std::shared_ptr <Miami::Disco::SafeLockGuard> &,
                                        ReaderWriterSwarmCheckCommons::Interval *output)
        {
            output->first = ReaderWriterSwarmCheckCommons::Clock::now ();
            std::this_thread::sleep_for (waitTime);
            output->second = ReaderWriterSwarmCheckCommons::Clock::now ();

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
            Miami::Disco::After (
                Miami::Disco::AnyLockPointer (&readWriteGuard.Read ()),
                FabricateAfterOneTestableCallback (baseCallback, finishes[readerIndex],
                                                   &swarmCheck.readersIntervals[readerIndex]));
        }

        for (uint32_t writerIndex = 0; writerIndex < ReaderWriterSwarmCheckCommons::writersCount; ++writerIndex)
        {
            Miami::Disco::After (
                Miami::Disco::AnyLockPointer (&readWriteGuard.Write ()),
                FabricateAfterOneTestableCallback (
                    baseCallback, finishes[ReaderWriterSwarmCheckCommons::readersCount + writerIndex],
                    &swarmCheck.writersIntervals[writerIndex]));
        }

        for (uint32_t taskIndex = 0; taskIndex < ReaderWriterSwarmCheckCommons::tasksCount; ++taskIndex)
        {
            finishes[taskIndex].get_future ().wait ();
        }

        swarmCheck.CheckResults ();
    }
}

BOOST_AUTO_TEST_CASE (AfterTwoSimpleLocksMutualExclusion)
{
    Miami::Disco::Context context {TEST_WORKERS_COUNT};
    {
        Miami::Disco::Lock firstLock {&context};
        Miami::Disco::Lock secondLock {&context};
        MutexCheckCommons checkData;

        std::function < void (std::vector <std::shared_ptr <Miami::Disco::SafeLockGuard>>, uint8_t) > taskBase =
            [&checkData] (const std::vector <std::shared_ptr <Miami::Disco::SafeLockGuard>> &guards, uint8_t filler)
            {
                for (uint32_t index = 0; index < MutexCheckCommons::chunkSize; ++index)
                {
                    checkData.storage.push_back (filler);
                }
            };

        std::promise <void> firstFinished;
        std::promise <void> secondFinished;

        Miami::Disco::After (
            {Miami::Disco::AnyLockPointer (&firstLock), Miami::Disco::AnyLockPointer (&secondLock)},
            FabricateAfterMultipleTestableCallback (taskBase, firstFinished, MutexCheckCommons::firstFiller));

        Miami::Disco::After (
            {Miami::Disco::AnyLockPointer (&firstLock), Miami::Disco::AnyLockPointer (&secondLock)},
            FabricateAfterMultipleTestableCallback (taskBase, secondFinished, MutexCheckCommons::secondFiller));

        firstFinished.get_future ().wait ();
        secondFinished.get_future ().wait ();
        checkData.CheckResults ();
    }
}

BOOST_AUTO_TEST_CASE (AfterTwoLocksCancel)
{
    Miami::Disco::Context context {TEST_WORKERS_COUNT};
    {
        Miami::Disco::Lock lock {&context};
        auto *readWriteGuard = new Miami::Disco::ReadWriteGuard {&context};
        BOOST_REQUIRE_MESSAGE(readWriteGuard->Write ().TryLock (),
                              "First write lock must be locked after creation.");

        std::promise <void> finished;
        bool canceled = false;

        std::function < void (std::vector <std::shared_ptr <Miami::Disco::SafeLockGuard>>) > onNext =
            [&finished] (const std::vector <std::shared_ptr <Miami::Disco::SafeLockGuard>> &)
            {
                finished.set_value ();
            };

        std::function < void () > onCancel = [&canceled, &finished]
        {
            canceled = true;
            finished.set_value ();
        };

        Miami::Disco::After (
            {Miami::Disco::AnyLockPointer (&lock), Miami::Disco::AnyLockPointer (&readWriteGuard->Read ())},
            onNext, onCancel);

        delete readWriteGuard;
        finished.get_future ().wait ();
        BOOST_REQUIRE(canceled);
    }
}

BOOST_AUTO_TEST_CASE (AfterTwoLocksCaptureLockGuard)
{
    Miami::Disco::Context context {TEST_WORKERS_COUNT};
    {
        Miami::Disco::Lock firstLock {&context};
        Miami::Disco::Lock secondLock {&context};
        std::shared_ptr <Miami::Disco::SafeLockGuard> capturedGuard = nullptr;
        std::promise <void> finished;

        std::function < void (std::vector <std::shared_ptr <Miami::Disco::SafeLockGuard>>) > onNext =
            [&capturedGuard, &finished] (const std::vector <std::shared_ptr <Miami::Disco::SafeLockGuard>> &guards)
            {
                capturedGuard = guards[0];
                finished.set_value ();
            };

        Miami::Disco::After (
            {Miami::Disco::AnyLockPointer (&firstLock), Miami::Disco::AnyLockPointer (&secondLock)}, onNext);
        finished.get_future ().wait ();

        BOOST_REQUIRE(capturedGuard);
        BOOST_REQUIRE_MESSAGE(!firstLock.TryLock (), "First lock should be captured even after task execution.");
        // TODO: In some super rare cases it fails. I wasn't able to debug it, but it seems that sometimes TryLock
        //       call enters kernel mode before SafeLockGuard destructor and therefore lock is not free at that moment.
        BOOST_REQUIRE_MESSAGE(secondLock.TryLock (), "Second lock should be free after task execution.");
    }
}

BOOST_AUTO_TEST_CASE (AfterMultipleGroupRequiresAllLocksCapture)
{
    Miami::Disco::Context context {TEST_WORKERS_COUNT};
    {
        Miami::Disco::Lock firstLock {&context};
        Miami::Disco::Lock secondLock {&context};

        BOOST_REQUIRE_MESSAGE(firstLock.TryLock (), "First lock must be locked after creation.");
        BOOST_REQUIRE_MESSAGE(secondLock.TryLock (), "Second lock must be locked after creation.");

        std::vector <uint8_t> output;
        const std::vector <uint8_t> firstExpectedOutput = {1u};
        const std::vector <uint8_t> secondExpectedOutput = {1u, 2u};
        const std::vector <uint8_t> thirdExpectedOutput = {1u, 2u, 3u};

        std::promise <void> oneAdded;
        std::promise <void> twoAdded;
        std::promise <void> threeAdded;

        std::function < void (std::vector <std::shared_ptr <Miami::Disco::SafeLockGuard>>) > bothLocksCallback =
            [&output, &threeAdded] (const std::vector <std::shared_ptr <Miami::Disco::SafeLockGuard>> &guards)
            {
                output.emplace_back (3u);
                threeAdded.set_value ();
            };

        std::function < void (std::shared_ptr <Miami::Disco::SafeLockGuard>, uint8_t) > inserterCallback =
            [&output] (const std::shared_ptr <Miami::Disco::SafeLockGuard> &guard, uint8_t value)
            {
                output.emplace_back (value);
            };

        Miami::Disco::After ({Miami::Disco::AnyLockPointer (&firstLock), Miami::Disco::AnyLockPointer (&secondLock)},
                             bothLocksCallback);

        Miami::Disco::After (Miami::Disco::AnyLockPointer (&firstLock),
                             FabricateAfterOneTestableCallback (inserterCallback, oneAdded, static_cast<uint8_t>(1u)));

        Miami::Disco::After (Miami::Disco::AnyLockPointer (&secondLock),
                             FabricateAfterOneTestableCallback (inserterCallback, twoAdded, static_cast<uint8_t>(2u)));

        firstLock.Unlock ();
        oneAdded.get_future ().wait ();
        BOOST_REQUIRE(output == firstExpectedOutput);

        BOOST_REQUIRE_MESSAGE(firstLock.TryLock (), "Master thread should be able to capture first lock again");
        secondLock.Unlock ();
        twoAdded.get_future ().wait ();
        BOOST_REQUIRE(output == secondExpectedOutput);

        firstLock.Unlock ();
        threeAdded.get_future ().wait ();
        BOOST_REQUIRE(output == thirdExpectedOutput);
    }
}

BOOST_AUTO_TEST_CASE (AfterCombinedReadWrite)
{
    class Object
    {
    public:
        Object (Miami::Disco::Context *context)
            : readWriteGuard_ (context)
        {
        }

        std::vector <uint8_t> storage_;
        uint64_t accumulator_ = 0;
        Miami::Disco::ReadWriteGuard readWriteGuard_;
    };

    Miami::Disco::Context context {TEST_WORKERS_COUNT};
    {
        Object first {&context};
        Object second {&context};

        first.accumulator_ = 1;
        second.storage_ = {1u, 2u, 3u};

        std::function <void (std::vector <std::shared_ptr <Miami::Disco::SafeLockGuard>>, Object *, Object *)>
            accumulatorAdder =
            [] (const std::vector <std::shared_ptr <Miami::Disco::SafeLockGuard>> &guards,
                Object *from, Object *to)
            {
                to->accumulator_ += from->accumulator_;
            };

        std::function <void (std::vector <std::shared_ptr <Miami::Disco::SafeLockGuard>>, Object *, Object *)>
            storagePusher =
            [] (const std::vector <std::shared_ptr <Miami::Disco::SafeLockGuard>> &guards,
                Object *from, Object *to)
            {
                to->storage_.insert (to->storage_.end (), from->storage_.begin (), from->storage_.end ());
            };

        constexpr uint32_t addersCount = 21;
        constexpr uint32_t pushersCount = 23;
        constexpr uint32_t totalTasks = addersCount + pushersCount;

        std::vector <std::promise <void>> finishers;
        for (uint32_t index = 0; index < totalTasks; ++index)
        {
            finishers.emplace_back ();
        }

        for (uint32_t adderIndex = 0; adderIndex < addersCount; ++adderIndex)
        {
            Miami::Disco::After (
                {
                    Miami::Disco::AnyLockPointer (&first.readWriteGuard_.Read ()),
                    Miami::Disco::AnyLockPointer (&second.readWriteGuard_.Write ())
                },
                FabricateAfterMultipleTestableCallback (accumulatorAdder, finishers[adderIndex], &first, &second));
        }

        for (uint32_t pusherIndex = 0; pusherIndex < pushersCount; ++pusherIndex)
        {
            Miami::Disco::After (
                {
                    Miami::Disco::AnyLockPointer (&second.readWriteGuard_.Read ()),
                    Miami::Disco::AnyLockPointer (&first.readWriteGuard_.Write ())
                },
                FabricateAfterMultipleTestableCallback (storagePusher, finishers[addersCount + pusherIndex],
                                                        &second, &first));
        }

        for (uint32_t index = 0; index < totalTasks; ++index)
        {
            finishers[index].get_future ().wait ();
        }

        BOOST_REQUIRE(second.accumulator_ == first.accumulator_ * addersCount);
        BOOST_REQUIRE(first.storage_.size () == second.storage_.size () * pushersCount);

        for (uint32_t index = 0; index < first.storage_.size (); ++index)
        {
            BOOST_REQUIRE(first.storage_[index] == second.storage_[index % second.storage_.size ()]);
        }
    }
}

BOOST_AUTO_TEST_SUITE_END ()