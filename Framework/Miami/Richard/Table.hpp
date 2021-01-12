#pragma once

#include <set>
#include <string>
#include <unordered_map>
#include <memory>

#include <Miami/Annotations.hpp>

#include <Miami/Disco/Disco.hpp>

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
    // TODO: Replace with flat hash map.
    using Row = std::unordered_map <AnyDataId, AnyDataContainer>;

    Table (Disco::Context *multithreadingContext, AnyDataId id, std::string name);

    ~Table ();

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

    free_call ResultCode AddColumn (const std::shared_ptr <Disco::SafeLockGuard> &writeGuard,
                                    const ColumnInfo &info, AnyDataId &outputId);

    free_call ResultCode RemoveColumn (const std::shared_ptr <Disco::SafeLockGuard> &writeGuard, AnyDataId id);

    free_call ResultCode AddIndex (const std::shared_ptr <Disco::SafeLockGuard> &writeGuard,
                                   const IndexInfo &info, AnyDataId &outputId);

    free_call ResultCode RemoveIndex (const std::shared_ptr <Disco::SafeLockGuard> &writeGuard, AnyDataId id);

    free_call ResultCode InsertRow (const std::shared_ptr <Disco::SafeLockGuard> &writeGuard, moved_in Row &row);

    free_call bool IsSafeToRemove (const std::shared_ptr <Disco::SafeLockGuard> &writeGuard) const;

private:
    free_call bool CheckReadOrWriteGuard (const std::shared_ptr <Disco::SafeLockGuard> &readOrWriteGuard) const;

    free_call bool CheckWriteGuard (const std::shared_ptr <Disco::SafeLockGuard> &writeGuard) const;

    free_call bool IsSafeToRemoveInternal () const;

    free_call ResultCode ValidateRowChanged (const Row &row) const;

    /// Applies row data, which must be previously validated by ::ValidateRowChanges.
    /// TODO: Extracted only because of code duplication, might be pure design decision.
    free_call void ApplyValidRowChanges (AnyDataId rowId, moved_in Row &row);

    /// TODO: Temporary helper method for column value accessors. Will be reworked with memory mapping support.
    free_call ResultCode GetColumnValue (AnyDataId columnId, AnyDataId rowId, const AnyDataContainer *&output) const;

    free_call ResultCode UpdateRow (AnyDataId rowId, moved_in Row &changedValues);

    free_call ResultCode DeleteRow (AnyDataId rowId);

    const AnyDataId id_;
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

    friend class TableReadCursor;

    friend class TableEditCursor;
};

class TableReadCursor
{
public:
    free_call ResultCode Advance (int64_t step);

    free_call ResultCode Get (AnyDataId columnId, const AnyDataContainer *&output) const;

protected:
    TableReadCursor (Table *table, IndexCursor *indexCursor);

    free_call ResultCode GetCurrentId (AnyDataId &output) const;

    // TODO: Due to how this cursor is written, it's usage even after table deletion will not lead to
    //       crashes, because base cursor will be invalidated. But maybe this mechanism is too weak.
    Table *const table_;

private:
    std::unique_ptr <IndexCursor> baseCursor_;

    friend class Table;
};

class TableEditCursor final : public TableReadCursor
{
public:
    free_call ResultCode Update (moved_in Table::Row &changedValues);

    free_call ResultCode DeleteCurrent ();

private:
    TableEditCursor (Table *table, IndexCursor *indexCursor);

    friend class Table;
};
}
