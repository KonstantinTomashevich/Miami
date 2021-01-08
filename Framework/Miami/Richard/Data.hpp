#pragma once

#include <array>
#include <cassert>
#include <cstdint>
#include <memory>
#include <variant>

#include <Miami/Annotations.hpp>

namespace Miami::Richard
{
using AnyDataId = uint64_t;

enum class DataType
{
    INT8 = 0,
    INT16,
    INT32,
    INT64,
    SHORT_STRING,
    STRING,
    LONG_STRING,
    HUGE_STRING,
    BLOB_16KB
};

constexpr uint32_t GetDataTypeSize (DataType dataType)
{
    switch (dataType)
    {
        case DataType::INT8:
            return 1u;

        case DataType::INT16:
            return 2u;

        case DataType::INT32:
            return 4u;

        case DataType::INT64:
            return 8u;

        case DataType::SHORT_STRING:
            return 16u;

        case DataType::STRING:
            return 64u;

        case DataType::LONG_STRING:
            return 256u;

        case DataType::HUGE_STRING:
            return 1024u;

        case DataType::BLOB_16KB:
            return 16384u;
    }

    assert (false);
    return 0;
}

template <DataType dataType>
struct DataTypeToCxxType
{
    using Type = std::array <uint8_t, GetDataTypeSize (dataType)>;
};

template <>
struct DataTypeToCxxType <DataType::INT8>
{
    using Type = int8_t;
};

template <>
struct DataTypeToCxxType <DataType::INT16>
{
    using Type = int16_t;
};

template <>
struct DataTypeToCxxType <DataType::INT32>
{
    using Type = int32_t;
};

template <>
struct DataTypeToCxxType <DataType::INT64>
{
    using Type = int64_t;
};

template <DataType dataType>
struct IsSmallDataType
{
    static constexpr bool answer = GetDataTypeSize (dataType) <= GetDataTypeSize (DataType::SHORT_STRING);
};

template <DataType dataType, bool = IsSmallDataType <dataType>::answer>
struct DataContainer;

template <DataType dataType>
struct DataContainer <dataType, true>
{
    typename DataTypeToCxxType <dataType>::Type value_;
};

template <DataType dataType>
struct DataContainer <dataType, false>
{
    std::unique_ptr <typename DataTypeToCxxType <dataType>::Type> value_;
};

class AnyDataContainer final
{
public:
    using Container = std::variant <
        DataContainer <DataType::INT8>,
        DataContainer <DataType::INT16>,
        DataContainer <DataType::INT32>,
        DataContainer <DataType::INT64>,
        DataContainer <DataType::SHORT_STRING>,
        DataContainer <DataType::STRING>,
        DataContainer <DataType::LONG_STRING>,
        DataContainer <DataType::HUGE_STRING>,
        DataContainer <DataType::BLOB_16KB>>;

    template <DataType dataType>
    explicit AnyDataContainer (moved_in typename DataTypeToCxxType <dataType>::Type &value)
        : container_ (std::move (value))
    {
    }

    DataType GetType () const;

    Container &Get ();

    const Container &Get () const;

private:
    Container container_;
};
}