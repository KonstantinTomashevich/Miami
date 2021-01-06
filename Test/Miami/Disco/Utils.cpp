#include "Utils.hpp"

#include <boost/test/unit_test.hpp>

void MutexCheckData::DoCheck () const
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
