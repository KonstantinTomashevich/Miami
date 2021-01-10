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

    Index (const std::pair <Table *, IndexInfo> &initializer);

    ~Index ();

    free_call bool IsSafeToRemove (const std::shared_ptr <Disco::SafeLockGuard> &tableWriteGuard) const;

    free_call const IndexInfo &GetIndexInfo () const;

    IndexCursor *OpenCursor ();

private:
    // On* methods do not check guards, because they could be called from implicitly guarded contexts like cursors.
    // Because of it, they are marked private, so they could be called only from safe context.

    free_call ResultCode OnInsert (AnyDataId insertedRowId);

    free_call ResultCode OnUpdate (AnyDataId updatedRowId);

    free_call ResultCode OnDelete (AnyDataId deletedRowId);

    void CloseCursor (IndexCursor *cursor);

    free_call bool IsSafeToRemoveInternal () const;

    free_call bool IsRowLess (AnyDataId firstRow, AnyDataId secondRow) const;

    IndexInfo info_;
    std::vector <AnyDataId> order_;
    std::mutex cursorManagementGuard_;
    std::vector <IndexCursor *> managedCursors_;
    Table *table_;

    // TODO: Add persistence (save/load from file).

    friend class IndexCursor;

    // TODO: Too many friend classes, too many private methods, reexamine decisions and maybe add proxies.
    friend class Table;
};
}