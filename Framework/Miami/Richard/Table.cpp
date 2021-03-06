#include <cassert>

#include <Miami/Evan/Logger.hpp>

#include <Miami/Richard/Table.hpp>

namespace Miami::Richard
{
Table::Table (Disco::Context *multithreadingContext, AnyDataId id, std::string name)
    : id_ (id),
      guard_ (multithreadingContext),
      name_ (std::move (name)),

      columns_ (),
      indices_ (),
      rows_ (),

      nextColumnId_ (0),
      nextIndexId_ (0),
      nextRowId_ (0)
{
}

Table::~Table ()
{
    assert (IsSafeToRemoveInternal ());
}

AnyDataId Table::GetId () const
{
    return id_;
}

Disco::ReadWriteGuard &Table::ReadWriteGuard ()
{
    return guard_;
}

ResultCode Table::GetName (const std::shared_ptr <Disco::SafeLockGuard> &readOrWriteGuard, std::string &output) const
{
    if (!CheckReadOrWriteGuard (readOrWriteGuard))
    {
        return ResultCode::INVARIANTS_VIOLATED;
    }

    output = name_;
    return ResultCode::OK;
}

ResultCode Table::CreateReadCursor (const std::shared_ptr <Disco::SafeLockGuard> &readOrWriteGuard, AnyDataId indexId,
                                    TableReadCursor *&output)
{
    if (!CheckReadOrWriteGuard (readOrWriteGuard))
    {
        return ResultCode::INVARIANTS_VIOLATED;
    }

    auto iterator = indices_.find (indexId);
    if (iterator == indices_.end ())
    {
        return ResultCode::INDEX_WITH_GIVEN_ID_NOT_FOUND;
    }

    output = new TableReadCursor (this, iterator->second.OpenCursor ());
    return ResultCode::OK;
}

ResultCode Table::GetColumnsIds (const std::shared_ptr <Disco::SafeLockGuard> &readOrWriteGuard,
                                 std::vector <AnyDataId> &output) const
{
    if (!CheckReadOrWriteGuard (readOrWriteGuard))
    {
        return ResultCode::INVARIANTS_VIOLATED;
    }

    for (const auto &idColumnPair : columns_)
    {
        output.emplace_back (idColumnPair.first);
    }

    return ResultCode::OK;
}

ResultCode Table::GetColumnInfo (const std::shared_ptr <Disco::SafeLockGuard> &readOrWriteGuard,
                                 AnyDataId id, ColumnInfo &output) const
{
    if (!CheckReadOrWriteGuard (readOrWriteGuard))
    {
        return ResultCode::INVARIANTS_VIOLATED;
    }

    auto iterator = columns_.find (id);

    if (iterator == columns_.end ())
    {
        return ResultCode::COLUMN_WITH_GIVEN_ID_NOT_FOUND;
    }
    else
    {
        output = iterator->second.GetColumnInfo ();
        return ResultCode::OK;
    }
}

ResultCode Table::GetIndicesIds (const std::shared_ptr <Disco::SafeLockGuard> &readOrWriteGuard,
                                 std::vector <AnyDataId> &output) const
{
    if (!CheckReadOrWriteGuard (readOrWriteGuard))
    {
        return ResultCode::INVARIANTS_VIOLATED;
    }

    for (const auto &idIndexPair : indices_)
    {
        output.emplace_back (idIndexPair.first);
    }

    return ResultCode::OK;
}

ResultCode Table::GetIndexInfo (const std::shared_ptr <Disco::SafeLockGuard> &readOrWriteGuard,
                                AnyDataId id, IndexInfo &output) const
{
    if (!CheckReadOrWriteGuard (readOrWriteGuard))
    {
        return ResultCode::INVARIANTS_VIOLATED;
    }

    auto iterator = indices_.find (id);
    if (iterator == indices_.end ())
    {
        return ResultCode::INDEX_WITH_GIVEN_ID_NOT_FOUND;
    }
    else
    {
        output = iterator->second.GetIndexInfo ();
        return ResultCode::OK;
    }
}

ResultCode Table::SetName (const std::shared_ptr <Disco::SafeLockGuard> &writeGuard, const std::string &name)
{
    if (!CheckWriteGuard (writeGuard))
    {
        return ResultCode::INVARIANTS_VIOLATED;
    }

    if (name.empty ())
    {
        return ResultCode::TABLE_NAME_SHOULD_NOT_BE_EMPTY;
    }
    else
    {
        name_ = name;
        return ResultCode::OK;
    }
}

ResultCode Table::CreateEditCursor (const std::shared_ptr <Disco::SafeLockGuard> &writeGuard, AnyDataId indexId,
                                    TableEditCursor *&output)
{
    if (!CheckWriteGuard (writeGuard))
    {
        return ResultCode::INVARIANTS_VIOLATED;
    }

    auto iterator = indices_.find (indexId);
    if (iterator == indices_.end ())
    {
        return ResultCode::INDEX_WITH_GIVEN_ID_NOT_FOUND;
    }

    output = new TableEditCursor (this, iterator->second.OpenCursor ());
    return ResultCode::OK;
}

ResultCode Table::AddColumn (const std::shared_ptr <Disco::SafeLockGuard> &writeGuard,
                             const ColumnInfo &info, AnyDataId &outputId)
{
    if (!CheckWriteGuard (writeGuard))
    {
        return ResultCode::INVARIANTS_VIOLATED;
    }

    if (info.name_.empty ())
    {
        return ResultCode::COLUMN_NAME_SHOULD_NOT_BE_EMPTY;
    }

    AnyDataId columnId = nextColumnId_++;
    if (columns_.count (columnId) > 0)
    {
        Evan::Logger::Get ().Log (Evan::LogLevel::ERROR, "Unable to add column to table \"" + name_ +
                                                         "\", because autogenerated column id is already used!");
        assert (false);
        return ResultCode::INVARIANTS_VIOLATED;
    }
    else
    {
        outputId = columnId;
        auto result = columns_.emplace (columnId, ColumnInfo {columnId, info.dataType_, info.name_});

        if (result.second)
        {
            return ResultCode::OK;
        }
        else
        {
            Evan::Logger::Get ().Log (Evan::LogLevel::ERROR, "Unable to add column to table \"" + name_ +
                                                             "\", because emplace operation failed!");
            assert (false);
            return ResultCode::INVARIANTS_VIOLATED;
        }
    }
}

ResultCode Table::RemoveColumn (const std::shared_ptr <Disco::SafeLockGuard> &writeGuard, AnyDataId id)
{
    if (!CheckWriteGuard (writeGuard))
    {
        return ResultCode::INVARIANTS_VIOLATED;
    }

    auto iterator = columns_.find (id);
    if (iterator == columns_.end ())
    {
        return ResultCode::COLUMN_WITH_GIVEN_ID_NOT_FOUND;
    }
    else
    {
        // Column removal is rare operation, so it's ok to iterate over all indices.
        std::vector <AnyDataId> cascadeIndices;

        for (auto &idIndexPair : indices_)
        {
            auto columnIdInIndex = std::find (idIndexPair.second.GetIndexInfo ().columns_.begin (),
                                              idIndexPair.second.GetIndexInfo ().columns_.end (), id);

            if (columnIdInIndex != idIndexPair.second.GetIndexInfo ().columns_.end ())
            {
                if (!idIndexPair.second.IsSafeToRemove (writeGuard))
                {
                    return ResultCode::COLUMN_REMOVAL_BLOCKED_BY_DEPENDANT_INDEX;
                }

                cascadeIndices.emplace_back (idIndexPair.first);
            }
        }

        columns_.erase (iterator);
        for (AnyDataId indexId : cascadeIndices)
        {
            indices_.erase (indexId);
        }

        return ResultCode::OK;
    }
}

ResultCode Table::AddIndex (const std::shared_ptr <Disco::SafeLockGuard> &writeGuard,
                            const IndexInfo &info, AnyDataId &outputId)
{
    if (!CheckWriteGuard (writeGuard))
    {
        return ResultCode::INVARIANTS_VIOLATED;
    }

    if (info.name_.empty ())
    {
        return ResultCode::INDEX_NAME_SHOULD_NOT_BE_EMPTY;
    }

    if (info.columns_.empty ())
    {
        return ResultCode::INDEX_MUST_DEPEND_ON_AT_LEAST_ONE_COLUMN;
    }

    for (AnyDataId columnId : info.columns_)
    {
        if (columns_.count (columnId) == 0)
        {
            Evan::Logger::Get ().Log (
                Evan::LogLevel::ERROR,
                "Unable to add index to table \"" + name_ + "\", because column with requested id " +
                std::to_string (columnId) + " is not found!");
            return ResultCode::COLUMN_WITH_GIVEN_ID_NOT_FOUND;
        }
    }

    AnyDataId indexId = nextIndexId_++;
    if (indices_.count (indexId) > 0)
    {
        Evan::Logger::Get ().Log (Evan::LogLevel::ERROR, "Unable to add index to table \"" + name_ +
                                                         "\", because autogenerated index id is already used!");
        assert (false);
        return ResultCode::INVARIANTS_VIOLATED;
    }
    else
    {
        outputId = indexId;
        auto result = indices_.emplace (
            indexId, std::make_pair (this, IndexInfo {indexId, info.name_, info.columns_}));

        if (result.second)
        {
            return ResultCode::OK;
        }
        else
        {
            Evan::Logger::Get ().Log (Evan::LogLevel::ERROR, "Unable to add index to table \"" + name_ +
                                                             "\", because emplace operation failed!");
            assert (false);
            return ResultCode::INVARIANTS_VIOLATED;
        }
    }
}

ResultCode Table::RemoveIndex (const std::shared_ptr <Disco::SafeLockGuard> &writeGuard, AnyDataId id)
{
    if (!CheckWriteGuard (writeGuard))
    {
        return ResultCode::INVARIANTS_VIOLATED;
    }

    auto iterator = indices_.find (id);
    if (iterator == indices_.end ())
    {
        return ResultCode::INDEX_WITH_GIVEN_ID_NOT_FOUND;
    }
    else
    {
        if (iterator->second.IsSafeToRemove (writeGuard))
        {
            indices_.erase (iterator);
            return ResultCode::OK;
        }
        else
        {
            return ResultCode::INDEX_REMOVAL_BLOCKED_BY_DEPENDANT_CURSORS;
        }
    }
}

ResultCode Table::InsertRow (const std::shared_ptr <Disco::SafeLockGuard> &writeGuard, Row &row)
{
    if (!CheckWriteGuard (writeGuard))
    {
        return ResultCode::INVARIANTS_VIOLATED;
    }

    AnyDataId rowId = nextRowId_++;
    if (rows_.count (rowId) > 0)
    {
        Evan::Logger::Get ().Log (Evan::LogLevel::ERROR, "Unable to add row to table \"" + name_ +
                                                         "\", because autogenerated row id is already used!");
        assert (false);
        return ResultCode::INVARIANTS_VIOLATED;
    }

    ResultCode validationResult = ValidateRowChanged (row);
    if (validationResult != ResultCode::OK)
    {
        --nextRowId_;
        return validationResult;
    }

    auto emplaceResult = rows_.emplace (rowId);
    if (emplaceResult.second)
    {
        ApplyValidRowChanges (rowId, row);
        for (auto &idIndexPair : indices_)
        {
            ResultCode resultCode = idIndexPair.second.OnInsert (rowId);
            if (resultCode != ResultCode::OK)
            {
                Evan::Logger::Get ().Log (
                    Evan::LogLevel::ERROR,
                    "Index \"" + idIndexPair.second.GetIndexInfo ().name_ + "\" of table \"" + name_ +
                    "\" is unable to process insertion of row " + std::to_string (rowId) + ", error " +
                    std::to_string (static_cast<uint64_t>(resultCode)) + "!");
                assert (false);
            }
        }

        return ResultCode::OK;
    }
    else
    {
        Evan::Logger::Get ().Log (Evan::LogLevel::ERROR, "Unable to add row to table \"" + name_ +
                                                         "\", because emplace operation failed!");
        assert (false);
        return ResultCode::INVARIANTS_VIOLATED;
    }
}

bool Table::IsSafeToRemove (const std::shared_ptr <Disco::SafeLockGuard> &writeGuard) const
{
    return CheckWriteGuard (writeGuard) && IsSafeToRemoveInternal ();
}

bool Table::CheckReadOrWriteGuard (const std::shared_ptr <Disco::SafeLockGuard> &readOrWriteGuard) const
{
    if (Disco::IsReadOrWriteCaptured (readOrWriteGuard, guard_))
    {
        return true;
    }
    else
    {
        Evan::Logger::Get ().Log (Evan::LogLevel::ERROR, "Caught attempt to read table \"" + name_ +
                                                         "\" data without proper read or write guard!");
        assert (false);
        return false;
    }
}

bool Table::CheckWriteGuard (const std::shared_ptr <Disco::SafeLockGuard> &writeGuard) const
{
    if (Disco::IsWriteCaptured (writeGuard, guard_))
    {
        return true;
    }
    else
    {
        Evan::Logger::Get ().Log (Evan::LogLevel::ERROR, "Caught attempt to edit table \"" + name_ +
                                                         "\" data without proper write guard!");
        assert (false);
        return false;
    }
}

bool Table::IsSafeToRemoveInternal () const
{
    for (auto &idIndexPair : indices_)
    {
        if (!idIndexPair.second.IsSafeToRemoveInternal ())
        {
            return false;
        }
    }

    return true;
}

ResultCode Table::ValidateRowChanged (const Table::Row &row) const
{
    for (const auto &columnDataPair : row)
    {
        auto columnIterator = columns_.find (columnDataPair.first);
        if (columnIterator == columns_.end ())
        {
            Evan::Logger::Get ().Log (
                Evan::LogLevel::ERROR,
                "Unable to add row to table \"" + name_ + "\", because column with requested id " +
                std::to_string (columnDataPair.first) + " is not found!");
            return ResultCode::COLUMN_WITH_GIVEN_ID_NOT_FOUND;
        }

        if (columnIterator->second.GetColumnInfo ().dataType_ != columnDataPair.second.GetType ())
        {
            Evan::Logger::Get ().Log (
                Evan::LogLevel::ERROR,
                "Unable to add row to table \"" + name_ + "\", because column with requested id " +
                std::to_string (columnDataPair.first) + " type is not equal to provided value type!");
            return ResultCode::NEW_COLUMN_VALUE_TYPE_MISMATCH;
        }
    }

    return ResultCode::OK;
}

void Table::ApplyValidRowChanges (AnyDataId rowId, Table::Row &row)
{
    for (auto &columnDataPair : row)
    {
        try
        {
            columns_.at (columnDataPair.first).values_[rowId] = std::move (columnDataPair.second);
        }
        catch (std::out_of_range &exception)
        {
            Evan::Logger::Get ().Log (
                Evan::LogLevel::ERROR,
                "Addition of new value to table \"" + name_ + "\" for column with id " +
                std::to_string (columnDataPair.first) + " failed, but all column checks passed!");
            assert (false);
        }
    }

    row.clear ();
}

ResultCode Table::GetColumnValue (AnyDataId columnId, AnyDataId rowId, const AnyDataContainer *&output) const
{
    auto columnIterator = columns_.find (columnId);
    if (columnIterator == columns_.end ())
    {
        return ResultCode::COLUMN_WITH_GIVEN_ID_NOT_FOUND;
    }

    auto rowIterator = columnIterator->second.values_.find (rowId);
    if (rowIterator == columnIterator->second.values_.end ())
    {
        // Absence of value is not an error.
        output = nullptr;
    }
    else
    {
        output = &rowIterator->second;
    }

    return ResultCode::OK;
}

ResultCode Table::UpdateRow (const std::shared_ptr <Disco::SafeLockGuard> &writeGuard,
                             AnyDataId rowId, Table::Row &changedValues)
{
    if (!CheckWriteGuard (writeGuard))
    {
        return ResultCode::INVARIANTS_VIOLATED;
    }

    auto rowIterator = rows_.find (rowId);
    if (rowIterator == rows_.end ())
    {
        return ResultCode::ROW_WITH_GIVEN_ID_NOT_FOUND;
    }

    ResultCode validationResult = ValidateRowChanged (changedValues);
    if (validationResult != ResultCode::OK)
    {
        return validationResult;
    }

    std::unordered_set <AnyDataId> changedColumns;
    for (auto &idValuePair : changedValues)
    {
        changedColumns.emplace (idValuePair.first);
    }

    ApplyValidRowChanges (rowId, changedValues);
    for (auto &idIndexPair : indices_)
    {
        bool anyIntersects = false;

        // Inform only indices, which use this column.
        for (auto columnId : idIndexPair.second.GetIndexInfo ().columns_)
        {
            if (changedColumns.count (columnId) > 0)
            {
                anyIntersects = true;
                break;
            }
        }

        if (anyIntersects)
        {
            ResultCode result = idIndexPair.second.OnUpdate (rowId);
            if (result != ResultCode::OK)
            {
                Evan::Logger::Get ().Log (
                    Evan::LogLevel::ERROR,
                    "Index \"" + idIndexPair.second.GetIndexInfo ().name_ + "\" of table \"" + name_ +
                    "\" unable to successfully process update of row " + std::to_string (rowId) + ", error " +
                    std::to_string (static_cast<uint64_t>(result)) + "!");
                assert (false);

                // TODO: For now, index OnInsert/OnUpdate/OnDelete failures are logger
                //       and skipped. Any thoughts about how to do it better?
            }
        }
    }

    return ResultCode::OK;
}

ResultCode Table::DeleteRow (const std::shared_ptr <Disco::SafeLockGuard> &writeGuard, AnyDataId rowId)
{
    if (!CheckWriteGuard (writeGuard))
    {
        return ResultCode::INVARIANTS_VIOLATED;
    }

    auto rowIterator = rows_.find (rowId);
    if (rowIterator == rows_.end ())
    {
        return ResultCode::ROW_WITH_GIVEN_ID_NOT_FOUND;
    }

    rows_.erase (rowIterator);
    for (auto &idColumnPair : columns_)
    {
        idColumnPair.second.values_.erase (rowId);
    }

    for (auto &idIndexPair : indices_)
    {
        ResultCode result = idIndexPair.second.OnDelete (rowId);
        if (result != ResultCode::OK)
        {
            Evan::Logger::Get ().Log (
                Evan::LogLevel::ERROR,
                "Index \"" + idIndexPair.second.GetIndexInfo ().name_ + "\" of table \"" + name_ +
                "\" unable to successfully process deletion of row " + std::to_string (rowId) + ", error " +
                std::to_string (static_cast<uint64_t>(result)) + "!");
            assert (false);
        }
    }

    return ResultCode::OK;
}

ResultCode TableReadCursor::Advance (const std::shared_ptr <Disco::SafeLockGuard> &readOrWriteGuard, int64_t step)
{
    assert(baseCursor_);
    if (baseCursor_ && table_->CheckReadOrWriteGuard (readOrWriteGuard))
    {
        return baseCursor_->Advance (step);
    }
    else
    {
        return ResultCode::INVARIANTS_VIOLATED;
    }
}

ResultCode TableReadCursor::Get (const std::shared_ptr <Disco::SafeLockGuard> &readOrWriteGuard,
                                 AnyDataId columnId, const AnyDataContainer *&output) const
{
    // If table was deleted, index deletion will invalidate base cursor, but there is no invalidation mechanism
    // for table cursors. Because of it, we should get current row id from base cursor to check if it is valid.
    AnyDataId currentId;
    ResultCode result = GetCurrentId (currentId);

    if (result != ResultCode::OK)
    {
        return result;
    }

    if (!table_->CheckReadOrWriteGuard (readOrWriteGuard))
    {
        return ResultCode::INVARIANTS_VIOLATED;
    }

    return table_->GetColumnValue (columnId, currentId, output);
}

TableReadCursor::TableReadCursor (Table *table, IndexCursor *indexCursor)
    : baseCursor_ (indexCursor),
      table_ (table)
{
    assert(baseCursor_);
    assert(table);
}

ResultCode TableReadCursor::GetCurrentId (AnyDataId &output) const
{
    assert(baseCursor_);
    if (baseCursor_)
    {
        return baseCursor_->GetCurrent (output);
    }
    else
    {
        return ResultCode::INVARIANTS_VIOLATED;
    }
}

ResultCode TableEditCursor::Update (const std::shared_ptr <Disco::SafeLockGuard> &writeGuard,
                                    Table::Row &changedValues)
{
    AnyDataId currentId;
    ResultCode result = GetCurrentId (currentId);

    if (result != ResultCode::OK)
    {
        return result;
    }

    return table_->UpdateRow (writeGuard, currentId, changedValues);
}

ResultCode TableEditCursor::DeleteCurrent (const std::shared_ptr <Disco::SafeLockGuard> &writeGuard)
{
    AnyDataId currentId;
    ResultCode result = GetCurrentId (currentId);

    if (result != ResultCode::OK)
    {
        return result;
    }

    return table_->DeleteRow (writeGuard, currentId);
}

TableEditCursor::TableEditCursor (Table *table, IndexCursor *indexCursor)
    : TableReadCursor (table, indexCursor)
{
}
}