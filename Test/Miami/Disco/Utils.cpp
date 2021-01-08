#include "Utils.hpp"

#include <array>

#include <boost/test/unit_test.hpp>
#include <boost/format.hpp>

void MutexCheckCommons::CheckResults () const
{
    BOOST_REQUIRE(storage.size () == chunkSize * 2);
    std::array <uint8_t, 2> expectedFillers {};

    if (storage[0] == firstFiller)
    {
        expectedFillers = {firstFiller, secondFiller};
    }
    else
    {
        expectedFillers = {secondFiller, firstFiller};
    }

    for (uint32_t index = 0; index < chunkSize * 2; ++index)
    {
        BOOST_REQUIRE(storage[index] == expectedFillers[index / chunkSize]);
    }
}

void ReaderWriterSwarmCheckCommons::CheckResults () const
{
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
