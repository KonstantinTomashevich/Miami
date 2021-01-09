#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <mutex>

#include <Miami/Annotations.hpp>

#include <Miami/Disco/Variants.hpp>

#include <Miami/Richard/Data.hpp>
#include <Miami/Richard/Errors.hpp>

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

    free_call Error Advance (int64_t step);

    free_call Error GetCurrent (AnyDataId &output) const;

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

    ~Index ();

    free_call void OnInsert (const std::shared_ptr <Disco::SafeLockGuard> &tableWriteGuard, AnyDataId insertedRowId);

    free_call void OnUpdate (const std::shared_ptr <Disco::SafeLockGuard> &tableWriteGuard, AnyDataId updatedRowId);

    free_call void OnDelete (const std::shared_ptr <Disco::SafeLockGuard> &tableWriteGuard, AnyDataId deletedRowId);

    free_call bool IsSafeToRemove (const std::shared_ptr <Disco::SafeLockGuard> &tableWriteGuard);

    IndexCursor *OpenCursor ();

private:
    void CloseCursor (IndexCursor *cursor);

    free_call void AssertTableWriteGuard (const std::shared_ptr <Disco::SafeLockGuard> &tableWriteGuard);
    free_call bool IsSafeToRemoveInternal ();
    free_call bool IsRowLess(AnyDataId firstRow, AnyDataId secondRaw) const;

    IndexInfo info_;
    std::vector <AnyDataId> order_;
    std::mutex cursorManagementGuard_;
    std::vector <IndexCursor *> managedCursors_;
    Table *table_;

    // TODO: Add persistence (save/load from file).

    friend class IndexCursor;
};
}