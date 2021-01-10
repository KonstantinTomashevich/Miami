#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <mutex>

#include <Miami/Annotations.hpp>

#include <Miami/Disco/Variants.hpp>

#include <Miami/Richard/Data.hpp>
#include <Miami/Richard/ResultCode.hpp>

namespace Miami::Richard
{
struct IndexInfo
{
    AnyDataId id_;
    std::string name_;
    std::vector <AnyDataId> columns_;
};

class Index;

class Table;

class IndexCursor final
{
public:
    ~IndexCursor ();

    free_call ResultCode Advance (int64_t step);

    free_call ResultCode GetCurrent (AnyDataId &output) const;

private:
    free_call IndexCursor (Index *sourceIndex, uint64_t position);

    free_call void AssertPosition () const;

    uint64_t position_;
    Index *sourceIndex_;

    friend class Index;
};

class Index final
{
public:
    Index (Table *table, IndexInfo info);

    Index (const std::pair<Table *, IndexInfo> &initializer);

    ~Index ();

    free_call ResultCode OnInsert (const std::shared_ptr <Disco::SafeLockGuard> &tableWriteGuard,
                                   AnyDataId insertedRowId);

    free_call ResultCode OnUpdate (const std::shared_ptr <Disco::SafeLockGuard> &tableWriteGuard,
                                   AnyDataId updatedRowId);

    free_call ResultCode OnDelete (const std::shared_ptr <Disco::SafeLockGuard> &tableWriteGuard,
                                   AnyDataId deletedRowId);

    free_call bool IsSafeToRemove (const std::shared_ptr <Disco::SafeLockGuard> &tableWriteGuard);

    free_call const IndexInfo &GetIndexInfo () const;

    IndexCursor *OpenCursor ();

private:
    void CloseCursor (IndexCursor *cursor);

    free_call bool IsSafeToRemoveInternal ();
    free_call bool IsRowLess(AnyDataId firstRow, AnyDataId secondRow) const;

    IndexInfo info_;
    std::vector <AnyDataId> order_;
    std::mutex cursorManagementGuard_;
    std::vector <IndexCursor *> managedCursors_;
    Table *table_;

    // TODO: Add persistence (save/load from file).

    friend class IndexCursor;
};
}