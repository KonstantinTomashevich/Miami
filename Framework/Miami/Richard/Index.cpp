#include <algorithm>
#include <cassert>

#include <Miami/Evan/Logger.hpp>

#include <Miami/Richard/Column.hpp>
#include <Miami/Richard/Index.hpp>

namespace Miami::Richard
{
IndexCursor::~IndexCursor ()
{
    assert (sourceIndex_);
    if (sourceIndex_)
    {
        sourceIndex_->CloseCursor (this);
    }
}

Error IndexCursor::Advance (int64_t step)
{
    assert (sourceIndex_);
    if (sourceIndex_)
    {
        AssertPosition ();
        if (step < 0)
        {
            if (-step > position_)
            {
                position_ = 0;
                return Error::CURSOR_ADVANCE_STOPPED_AT_BEGIN;
            }
            else
            {
                position_ += step;
                return Error::OK;
            }
        }
        else
        {
            position_ += step;
            if (position_ > sourceIndex_->order_.size ())
            {
                position_ = sourceIndex_->order_.size ();
                return Error::CURSOR_ADVANCE_STOPPED_AT_END;
            }
            else
            {
                return Error::OK;
            }
        }
    }
    else
    {
        return Error::INVARIANTS_VIOLATED;
    }
}

Error IndexCursor::GetCurrent (AnyDataId &output) const
{
    assert (sourceIndex_);
    if (sourceIndex_)
    {
        AssertPosition ();
        if (position_ < sourceIndex_->order_.size ())
        {
            output = sourceIndex_->order_[position_];
            return Error::OK;
        }
        else
        {
            return Error::CURSOR_GET_CURRENT_UNABLE_TO_GET_FROM_END;
        }
    }
    else
    {
        return Error::INVARIANTS_VIOLATED;
    }
}

IndexCursor::IndexCursor (Index *sourceIndex, uint64_t position)
    : sourceIndex_ (sourceIndex),
      position_ (position)
{
    assert (sourceIndex);
    AssertPosition ();
}

void IndexCursor::AssertPosition () const
{
    assert (position_ <= sourceIndex_->order_.size ());
}

Index::Index (Table *table, IndexInfo info)
    : info_ (std::move (info)),
      order_ (),
      cursorManagementGuard_ (),
      managedCursors_ (),
      table_ (table)
{
    assert (!info_.columns_.empty ());
    assert (!info_.name_.empty ());
    assert (table_);

    if (table_)
    {
        order_.reserve (table_->rows_.size ());
        for (AnyDataId rowId : table_->rows_)
        {
            order_.emplace_back (rowId);
        }

        std::sort (order_.begin (), order_.end (),
                   [this] (AnyDataId firstRow, AnyDataId secondRow)
                   {
                       return IsRowLess (firstRow, secondRow);
                   });
    }
    else
    {
        Evan::Logger::Get ().Log (
            Evan::LogLevel::ERROR,
            "Caught attempt to create index \"" + info.name_ + "\" without table pointer!");
    }

    if (info_.columns_.empty ())
    {
        Evan::Logger::Get ().Log (
            Evan::LogLevel::ERROR,
            "Caught attempt to create index \"" + info.name_ + "\" without indexed columns!");
    }
}

Index::~Index ()
{
    // We don't lock cursor management guard here, because index destruction could only be initiated
    // by thread with table write access. I hope, this uncheckable from here invariant won't be broken.
    assert (IsSafeToRemoveInternal ());

    // If removal was unsafe, we must nullify all active cursors.
    for (IndexCursor *cursor : managedCursors_)
    {
        assert (cursor);
        if (cursor)
        {
            cursor->sourceIndex_ = nullptr;
        }
    }
}

void Index::OnInsert (const std::shared_ptr <Disco::SafeLockGuard> &tableWriteGuard, AnyDataId insertedRowId)
{
    AssertTableWriteGuard (tableWriteGuard);
    // We don't lock cursor management guard here, because deletion callback could only be called by
    // thread with table write access. I hope, this uncheckable from here invariant won't be broken.

    auto iterator = std::lower_bound (order_.begin (), order_.end (), insertedRowId,
                                      [this] (AnyDataId firstRow, AnyDataId secondRow)
                                      {
                                          return IsRowLess (firstRow, secondRow);
                                      });

    if (iterator != order_.end () && *iterator == insertedRowId)
    {
        Evan::Logger::Get ().Log (
            Evan::LogLevel::ERROR,
            "Caught attempt to inform index \"" + info_.name_ + "\" about insertion of item \"" +
            std::to_string (insertedRowId) + ", but there is already item with such id in order vector!");
        assert (false);

        // As fallback behaviour, treat such inserts as updates.
        OnUpdate (tableWriteGuard, insertedRowId);
    }
    else
    {
        uint64_t position = std::distance (order_.begin (), iterator);
        for (IndexCursor *cursor : managedCursors_)
        {
            assert (cursor);
            if (cursor && cursor->position_ >= position)
            {
                ++cursor->position_;
            }
        }

        order_.insert (iterator, insertedRowId);
    }
}

void Index::OnUpdate (const std::shared_ptr <Disco::SafeLockGuard> &tableWriteGuard, AnyDataId updatedRowId)
{
    AssertTableWriteGuard (tableWriteGuard);
    // We don't lock cursor management guard here, because deletion callback could only be called by
    // thread with table write access. I hope, this uncheckable from here invariant won't be broken.

    // Simplest strategy for update processing is delete-insert combination.
    // And there is no better without explicit list of updated columns.
    OnDelete (tableWriteGuard, updatedRowId);
    OnInsert (tableWriteGuard, updatedRowId);
}

void Index::OnDelete (const std::shared_ptr <Disco::SafeLockGuard> &tableWriteGuard, AnyDataId deletedRowId)
{
    AssertTableWriteGuard (tableWriteGuard);
    // We don't lock cursor management guard here, because deletion callback could only be called by
    // thread with table write access. I hope, this uncheckable from here invariant won't be broken.

    auto iterator = std::lower_bound (order_.begin (), order_.end (), deletedRowId,
                                      [this] (AnyDataId firstRow, AnyDataId secondRow)
                                      {
                                          return IsRowLess (firstRow, secondRow);
                                      });

    if (iterator == order_.end () || *iterator != deletedRowId)
    {
        Evan::Logger::Get ().Log (
            Evan::LogLevel::ERROR,
            "Caught attempt to inform index \"" + info_.name_ + "\" about deletion of item \"" +
            std::to_string (deletedRowId) + ", but there is no such item in order vector!");
        assert (false);
    }
    else
    {
        uint64_t position = std::distance (order_.begin (), iterator);
        for (IndexCursor *cursor : managedCursors_)
        {
            assert (cursor);
            if (cursor && cursor->position_ > position)
            {
                --cursor->position_;
            }
        }

        auto nextIterator = order_.erase (iterator);
        if (nextIterator != order_.end () && *nextIterator == deletedRowId)
        {
            Evan::Logger::Get ().Log (
                Evan::LogLevel::ERROR,
                "While processing information about deletion of item " +
                std::to_string (deletedRowId) + " in index \"" + info_.name_ +
                "\", caught that there were more than one occurrence of that item in this index's order vector!");
            assert (false);

            // As fallback behaviour, remove all duplicates.
            while (nextIterator != order_.end () && *nextIterator == deletedRowId)
            {
                nextIterator = order_.erase (nextIterator);
            }
        }
    }
}

bool Index::IsSafeToRemove (const std::shared_ptr <Disco::SafeLockGuard> &tableWriteGuard)
{
    AssertTableWriteGuard (tableWriteGuard);
    return IsSafeToRemoveInternal ();
}

IndexCursor *Index::OpenCursor ()
{
    std::unique_lock <std::mutex> lock (cursorManagementGuard_);
    auto *cursor = new IndexCursor (this, 0);
    managedCursors_.emplace_back (cursor);
    return cursor;
}

void Index::CloseCursor (IndexCursor *cursor)
{
    assert (cursor);
    std::unique_lock <std::mutex> lock (cursorManagementGuard_);
    auto iterator = std::find (managedCursors_.begin (), managedCursors_.end (), cursor);

    if (iterator == managedCursors_.end ())
    {
        assert (false);
    }
    else
    {
        managedCursors_.erase (iterator);
    }
}

void Index::AssertTableWriteGuard (const std::shared_ptr <Disco::SafeLockGuard> &tableWriteGuard)
{
    assert (tableWriteGuard);
    assert (tableWriteGuard->Is (&table_->guard_.Write ()));
}

bool Index::IsSafeToRemoveInternal ()
{
    return managedCursors_.empty ();
}

bool Index::IsRowLess (AnyDataId firstRow, AnyDataId secondRaw) const
{
    assert(!info_.columns_.empty ());
    assert(table_);

    if (table_)
    {
        for (AnyDataId columnId : info_.columns_)
        {
            AnyDataContainer *firstValue = table_->GetColumnValue (columnId, firstRow);
            AnyDataContainer *secondValue = table_->GetColumnValue (columnId, secondRow);

            // Null is less than anything.
            if (firstValue == nullptr && secondValue != nullptr)
            {
                return true;
            }
            else if (firstValue != nullptr && secondValue == nullptr)
            {
                return false;
            }
            else if (firstValue != nullptr && secondValue != nullptr)
            {
                assert(firstValue->GetType () == secondValue->GetType ());

                bool less = *firstValue < *secondValue;
                if (less)
                {
                    return true;
                }
            }
        }
    }

    // If rows are equal, compare them by ids.
    return firstRow < secondRaw;
}
}