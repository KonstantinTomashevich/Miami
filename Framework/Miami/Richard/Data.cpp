#include <Miami/Richard/Data.hpp>

namespace Miami::Richard
{
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