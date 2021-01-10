#pragma once

#include <set>
#include <string>
#include <unordered_map>

#include <Miami/Annotations.hpp>

#include <Miami/Disco/Locks.hpp>

#include <Miami/Richard/Column.hpp>
#include <Miami/Richard/Data.hpp>
#include <Miami/Richard/Index.hpp>
#include <Miami/Richard/ResultCode.hpp>

namespace Miami::Richard
{
class TableReadCursor;

class TableEditCursor;

class Table final
{
public:
    Table (Disco::Context *multithreadingContext, AnyDataId id, std::string name);

    ~Table () = default;

    free_call AnyDataId GetId () const;

    free_call Disco::ReadWriteGuard &ReadWriteGuard ();

    free_call ResultCode GetName (const std::shared_ptr <Disco::SafeLockGuard> &readOrWriteGuard,
                                  std::string &output) const;

    free_call ResultCode CreateReadCursor (const std::shared_ptr <Disco::SafeLockGuard> &readOrWriteGuard,
                                           AnyDataId indexId, TableReadCursor *&output);

    free_call ResultCode GetColumnsIds (const std::shared_ptr <Disco::SafeLockGuard> &readOrWriteGuard,
                                        std::vector <AnyDataId> &output) const;

    free_call ResultCode GetColumnInfo (const std::shared_ptr <Disco::SafeLockGuard> &readOrWriteGuard,
                                        AnyDataId id, ColumnInfo &output) const;

    free_call ResultCode GetIndicesIds (const std::shared_ptr <Disco::SafeLockGuard> &readOrWriteGuard,
                                        std::vector <AnyDataId> &output) const;

    free_call ResultCode GetIndexInfo (const std::shared_ptr <Disco::SafeLockGuard> &readOrWriteGuard,
                                       AnyDataId id, IndexInfo &output) const;

    free_call ResultCode SetName (const std::shared_ptr <Disco::SafeLockGuard> &writeGuard, const std::string &name);

    free_call ResultCode CreateEditCursor (const std::shared_ptr <Disco::SafeLockGuard> &writeGuard,
                                           AnyDataId indexId, TableEditCursor *&output);

    free_call ResultCode AddColumn (const std::shared_ptr <Disco::SafeLockGuard> &writeGuard, const ColumnInfo &info);

    free_call ResultCode RemoveColumn (const std::shared_ptr <Disco::SafeLockGuard> &writeGuard, AnyDataId id);

    free_call ResultCode AddIndex (const std::shared_ptr <Disco::SafeLockGuard> &writeGuard, const IndexInfo &info);

    free_call ResultCode RemoveIndex (const std::shared_ptr <Disco::SafeLockGuard> &writeGuard, AnyDataId id);

    free_call ResultCode InsertRow (const std::shared_ptr <Disco::SafeLockGuard> &writeGuard,
                                    moved_in std::unordered_map <AnyDataId, AnyDataContainer> &values);

private:
    free_call bool CheckReadOrWriteGuard (const std::shared_ptr <Disco::SafeLockGuard> &readOrWriteGuard) const;

    free_call bool CheckWriteGuard (const std::shared_ptr <Disco::SafeLockGuard> &writeGuard) const;

    /// TODO: Temporary helper method for column value accessors. Will be reworked with memory mapping support.
    free_call AnyDataContainer *GetColumnValue (AnyDataId columnId, AnyDataId rowId);

    AnyDataId id_;
    Disco::ReadWriteGuard guard_;
    std::string name_;

    // TODO: Replace with flat maps and flat sets (from abseil, for example).
    std::unordered_map <AnyDataId, Column> columns_;
    std::unordered_map <AnyDataId, Index> indices_;
    std::set <AnyDataId> rows_;

    AnyDataId nextColumnId_;
    AnyDataId nextIndexId_;
    AnyDataId nextRowId_;

    friend class Index;
};
}
