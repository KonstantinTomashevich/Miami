#include <Miami/Richard/Column.hpp>

namespace Miami::Richard
{
Column::Column (ColumnInfo info)
    : info_ (std::move (info)),
      values_ ()
{

}

const ColumnInfo &Column::GetColumnInfo () const
{
    return info_;
}
}