#include <Miami/Richard/Data.hpp>

namespace Miami::Richard
{
DataType AnyDataContainer::GetType () const
{
    return static_cast<DataType>(container_.index ());
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
}