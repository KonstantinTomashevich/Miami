#include <Miami/Richard/Data.hpp>

namespace Miami::Richard
{
DataType AnyDataContainer::GetType () const
{
    return static_cast<DataType>(container_.index ());
}
}