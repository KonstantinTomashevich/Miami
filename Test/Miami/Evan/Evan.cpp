#include <boost/test/unit_test.hpp>
#include <boost/format.hpp>

#include <array>
#include <cstdint>
#include <thread>
#include <future>
#include <regex>

#include <Miami/Evan/Logger.hpp>
#include <Miami/Evan/GlobalLogger.hpp>

namespace Miami::Evan
{
class GlobalLoggerTestAccess
{
public:
    static void ClearOutputs ()
    {
        std::unique_lock <std::mutex> lock (GlobalLogger::Instance ().guard_);
        GlobalLogger::Instance ().outputs_.clear ();
    }
};
}

BOOST_AUTO_TEST_SUITE (Evan)

std::string CleanLogForTest (const std::string &log)
{
    static const std::regex logThreadAndTimeRemovedRegex {R"(\[.+\]\s)"};
    std::string result;
    std::regex_replace (
        std::back_inserter (result), log.begin (), log.end (), logThreadAndTimeRemovedRegex, "");
    return result;
}

class DeterministicLogCheck
{
public:
    DeterministicLogCheck (std::string expectedResult, Miami::Evan::LogLevel outputLevel)
        : logOutput_ (std::make_shared <std::stringstream> ()),
          expectedResult_ (std::move (expectedResult))
    {
        Miami::Evan::GlobalLogger::Instance ().AddOutput (logOutput_, outputLevel);
    }

    ~DeterministicLogCheck ()
    {
        Miami::Evan::GlobalLoggerTestAccess::ClearOutputs ();
        std::string log = logOutput_->str ();
        BOOST_TEST_MESSAGE(boost::format ("Log content:\n%1%") % log);

        std::string cleanLog = CleanLogForTest (log);
        BOOST_TEST_MESSAGE(boost::format ("Clean log content:\n%1%") % cleanLog);

        BOOST_TEST_MESSAGE(boost::format ("Expected log content:\n%1%") % expectedResult_);
        BOOST_REQUIRE (expectedResult_ == cleanLog);
    }

    std::shared_ptr <std::stringstream> logOutput_;
    std::string expectedResult_;
};

BOOST_AUTO_TEST_CASE (JustOutputFromSeparateThread)
{
    DeterministicLogCheck check ("VERBOSE Hello, world!\n", Miami::Evan::LogLevel::VERBOSE);
    std::thread separateThread (
        []
        {
            Miami::Evan::Logger::Get ().Log (Miami::Evan::LogLevel::VERBOSE, "Hello, world!");
        });

    separateThread.join ();
}

BOOST_AUTO_TEST_CASE (NoOutputWithoutThreadExitAndBufferOverflow)
{
    DeterministicLogCheck check ("", Miami::Evan::LogLevel::VERBOSE);
    Miami::Evan::Logger::Get ().Log (Miami::Evan::LogLevel::VERBOSE, "Hello, world!");
}

BOOST_AUTO_TEST_CASE (OutputAfterBufferOverflow)
{
    std::string expected;
    static const char *logMessage = "Hello, world!";

    for (uint32_t index = 0; index < Miami::Evan::Logger::MAX_BUFFER_SIZE; ++index)
    {
        expected += "VERBOSE ";
        expected += logMessage;
        expected += "\n";
    }

    DeterministicLogCheck check (expected, Miami::Evan::LogLevel::VERBOSE);
    for (uint32_t index = 0; index < Miami::Evan::Logger::MAX_BUFFER_SIZE; ++index)
    {
        Miami::Evan::Logger::Get ().Log (Miami::Evan::LogLevel::VERBOSE, logMessage);
    }
}

BOOST_AUTO_TEST_CASE (FilterOutByLogLevel)
{
    DeterministicLogCheck check ("", Miami::Evan::LogLevel::ERROR);
    std::thread separateThread (
        []
        {
            Miami::Evan::Logger::Get ().Log (Miami::Evan::LogLevel::VERBOSE, "Hello, world!");
        });

    separateThread.join ();
}

BOOST_AUTO_TEST_CASE (FilterInByLogLevel)
{
    DeterministicLogCheck check ("WARNING Hello, world!\n", Miami::Evan::LogLevel::INFO);
    std::thread separateThread (
        []
        {
            Miami::Evan::Logger::Get ().Log (Miami::Evan::LogLevel::WARNING, "Hello, world!");
        });

    separateThread.join ();
}

BOOST_AUTO_TEST_CASE (MultipleThreads)
{
    auto logMessageNTimes =
        [] (uint32_t times, const std::string &message)
        {
            for (uint32_t index = 0; index < times; ++index)
            {
                Miami::Evan::Logger::Get ().Log (Miami::Evan::LogLevel::VERBOSE, message);
            }
        };

    auto logOutput = std::make_shared <std::stringstream> ();
    Miami::Evan::GlobalLogger::Instance ().AddOutput (logOutput, Miami::Evan::LogLevel::VERBOSE);

    constexpr uint32_t threadsCount = 4;
    const std::array <std::string, threadsCount> messages
        {
            "First",
            "Second",
            "Third",
            "Fourth"
        };

    const std::array <uint32_t, threadsCount> expectedCounts
        {
            Miami::Evan::Logger::MAX_BUFFER_SIZE + 1,
            Miami::Evan::Logger::MAX_BUFFER_SIZE / 2,
            Miami::Evan::Logger::MAX_BUFFER_SIZE * 2,
            Miami::Evan::Logger::MAX_BUFFER_SIZE - 1
        };

    std::array <uint32_t, threadsCount> actualCounts {0, 0, 0, 0};
    std::vector <std::thread> threads;

    for (uint32_t threadIndex = 0; threadIndex < threadsCount; ++threadIndex)
    {
        threads.emplace_back (std::thread {logMessageNTimes, expectedCounts[threadIndex], messages[threadIndex]});
    }

    for (uint32_t threadIndex = 0; threadIndex < threadsCount; ++threadIndex)
    {
        threads[threadIndex].join ();
    }

    Miami::Evan::GlobalLoggerTestAccess::ClearOutputs ();
    {
        std::string log = logOutput->str ();
        BOOST_TEST_MESSAGE(boost::format ("Log content:\n%1%") % log);
    }

    // There is no std::string::ends_with in C++17 and there is no
    // need for it in frameworks, so it exists only inside this test.
    auto endsWith = [] (const std::string &string, const std::string &suffix)
    {
        return string.size () >= suffix.size () &&
               string.compare (string.size () - suffix.size (), suffix.size (), suffix) == 0;
    };

    std::string line;
    while (std::getline (*logOutput, line, '\n'))
    {
        for (uint32_t messageIndex = 0; messageIndex < threadsCount; ++messageIndex)
        {
            if (endsWith(line, messages[messageIndex]))
            {
                ++actualCounts[messageIndex];
                break;
            }
        }
    }

    BOOST_REQUIRE(actualCounts == expectedCounts);
}

BOOST_AUTO_TEST_SUITE_END ()