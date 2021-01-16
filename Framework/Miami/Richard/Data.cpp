#include <Miami/Richard/Data.hpp>

namespace Miami::Richard
{
AnyDataContainer::AnyDataContainer ()
    : container_ ()
{

}

AnyDataContainer::AnyDataContainer (DataType dataType)
    : container_ ()
{
    // TODO: Temporary adhok stub. There should be better solution.
    switch (dataType)
    {
        case DataType::INT8:
            container_ = DataContainer <DataType::INT8> ();
            break;

        case DataType::INT16:
            container_ = DataContainer <DataType::INT16> ();
            break;

        case DataType::INT32:
            container_ = DataContainer <DataType::INT32> ();
            break;

        case DataType::INT64:
            container_ = DataContainer <DataType::INT64> ();
            break;

        case DataType::SHORT_STRING:
            container_ = DataContainer <DataType::SHORT_STRING> ();
            break;

        case DataType::STRING:
            container_ = DataContainer <DataType::STRING> ();
            break;

        case DataType::LONG_STRING:
            container_ = DataContainer <DataType::LONG_STRING> ();
            break;

        case DataType::HUGE_STRING:
            container_ = DataContainer <DataType::HUGE_STRING> ();
            break;

        case DataType::BLOB_16KB:
            container_ = DataContainer <DataType::BLOB_16KB> ();
            break;
    }
}

DataType AnyDataContainer::GetType () const
{
    return static_cast<DataType>(container_.index ());
}

AnyDataContainer::Container &AnyDataContainer::Get ()
{
    return container_;
}

const AnyDataContainer::Container &AnyDataContainer::Get () const
{
    return container_;
}

bool AnyDataContainer::operator == (const AnyDataContainer &another) const
{
    return container_ == another.container_;
}

bool AnyDataContainer::operator != (const AnyDataContainer &another) const
{
    return !(another == *this);
}

bool AnyDataContainer::operator < (const AnyDataContainer &another) const
{
    return container_ < another.container_;
}

bool AnyDataContainer::operator > (const AnyDataContainer &another) const
{
    return another < *this;
}

bool AnyDataContainer::operator <= (const AnyDataContainer &another) const
{
    return !(another < *this);
}

bool AnyDataContainer::operator >= (const AnyDataContainer &another) const
{
    return !(*this < another);
}

void *AnyDataContainer::GetDataStartPointer ()
{
    return const_cast<void *>(const_cast<const AnyDataContainer *>(this)->GetDataStartPointer ());
}

const void *AnyDataContainer::GetDataStartPointer () const
{
    return std::visit ([] (const auto &value) -> const void *
                       {
                           return static_cast <const void *>(value.GetData ());
                       }, container_);
}
}