#pragma once

#include <string>
#include <unordered_map>

#include <Miami/Richard/Data.hpp>

namespace Miami::Richard
{
struct ColumnInfo
{
    AnyDataId id_;
    DataType dataType_;
    std::string name_;
};

class Column final
{
public:
    explicit Column (ColumnInfo info);

    const ColumnInfo &GetColumnInfo () const;

private:
    ColumnInfo info_;

    // TODO: Add associated header file and its management.

    // TODO: Temporary solution. Will be replaced with something like memory mapped files later.
    std::unordered_map <AnyDataId, AnyDataContainer> values_;

    // It's easier to implement value read/write process with good performance
    // inside table to manage all columns for required rows at once.
    friend class Table;
};
}