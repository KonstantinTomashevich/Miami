// Not only we don't need GDI, but it also has ERROR macro that breaks Evan's LogLevel.
#define NOGDI

#include <cassert>

#include <App/Miami/Server/Processing.hpp>

#include <Miami/Evan/Logger.hpp>

namespace Miami::App::Server
{
using namespace Messaging;

namespace Details
{
void SendVoidResult (const ProcessingContext &context, QueryId id, OperationResult result)
{
    assert (context.session_);
    VoidOperationResultResponse {id, result}.Write (
        Message::VOID_OPERATION_RESULT_RESPONSE, context.session_);
}

bool ExtractConstSessionExtension (const ProcessingContext &context, QueryId id,
                                   const std::shared_ptr <Disco::SafeLockGuard> &guard,
                                   const SessionExtension *&output)
{
    assert (context.session_);
    Hotline::ResultCode resultCode = context.session_->Data ().GetEntry <SessionExtension> (guard, output);

    if (resultCode == Hotline::ResultCode::OK)
    {
        return true;
    }
    else
    {
        Evan::Logger::Get ().Log (
            Evan::LogLevel::ERROR, "Caught error with code " +
                                   std::to_string (static_cast <uint64_t> (resultCode)) +
                                   " during session extension const extraction!");
        SendVoidResult (context, id, OperationResult::INTERNAL_ERROR);
        return false;
    }
}

bool ExtractSessionExtension (const ProcessingContext &context, QueryId id,
                              const std::shared_ptr <Disco::SafeLockGuard> &guard,
                              SessionExtension *&output)
{
    assert (context.session_);
    Hotline::ResultCode resultCode = context.session_->Data ().GetEntryForWrite <SessionExtension> (guard, output);

    if (resultCode == Hotline::ResultCode::OK)
    {
        return true;
    }
    else
    {
        Evan::Logger::Get ().Log (
            Evan::LogLevel::ERROR, "Caught error with code " +
                                   std::to_string (static_cast <uint64_t> (resultCode)) +
                                   " during session extension extraction for write!");
        SendVoidResult (context, id, OperationResult::INTERNAL_ERROR);
        return false;
    }
}

OperationResult MapDatabaseResultToOperationResult (Richard::ResultCode result)
{
    switch (result)
    {
        case Richard::ResultCode::OK:
            return OperationResult::OK;

        case Richard::ResultCode::CURSOR_ADVANCE_STOPPED_AT_BEGIN:
            return OperationResult::CURSOR_ADVANCE_STOPPED_AT_BEGIN;

        case Richard::ResultCode::CURSOR_ADVANCE_STOPPED_AT_END:
            return OperationResult::CURSOR_ADVANCE_STOPPED_AT_END;

        case Richard::ResultCode::CURSOR_GET_CURRENT_UNABLE_TO_GET_FROM_END:
            return OperationResult::CURSOR_GET_CURRENT_UNABLE_TO_GET_FROM_END;

        case Richard::ResultCode::COLUMN_WITH_GIVEN_ID_NOT_FOUND:
            return OperationResult::COLUMN_WITH_GIVEN_ID_NOT_FOUND;

        case Richard::ResultCode::INDEX_WITH_GIVEN_ID_NOT_FOUND:
            return OperationResult::INDEX_WITH_GIVEN_ID_NOT_FOUND;

        case Richard::ResultCode::ROW_WITH_GIVEN_ID_NOT_FOUND:
            return OperationResult::ROW_WITH_GIVEN_ID_NOT_FOUND;

        case Richard::ResultCode::TABLE_WITH_GIVEN_ID_NOT_FOUND:
            return OperationResult::TABLE_WITH_GIVEN_ID_NOT_FOUND;

        case Richard::ResultCode::TABLE_NAME_SHOULD_NOT_BE_EMPTY:
            return OperationResult::TABLE_NAME_SHOULD_NOT_BE_EMPTY;

        case Richard::ResultCode::COLUMN_NAME_SHOULD_NOT_BE_EMPTY:
            return OperationResult::COLUMN_NAME_SHOULD_NOT_BE_EMPTY;

        case Richard::ResultCode::INDEX_NAME_SHOULD_NOT_BE_EMPTY:
            return OperationResult::INDEX_NAME_SHOULD_NOT_BE_EMPTY;

        case Richard::ResultCode::INDEX_MUST_DEPEND_ON_AT_LEAST_ONE_COLUMN:
            return OperationResult::INDEX_MUST_DEPEND_ON_AT_LEAST_ONE_COLUMN;

        case Richard::ResultCode::COLUMN_REMOVAL_BLOCKED_BY_DEPENDANT_INDEX:
            return OperationResult::COLUMN_REMOVAL_BLOCKED_BY_DEPENDANT_INDEX;

        case Richard::ResultCode::INDEX_REMOVAL_BLOCKED_BY_DEPENDANT_CURSORS:
            return OperationResult::INDEX_REMOVAL_BLOCKED_BY_DEPENDANT_CURSORS;

        case Richard::ResultCode::TABLE_REMOVAL_BLOCKED:
            return OperationResult::TABLE_REMOVAL_BLOCKED;

        case Richard::ResultCode::NEW_COLUMN_VALUE_TYPE_MISMATCH:
            return OperationResult::NEW_COLUMN_VALUE_TYPE_MISMATCH;

        default:
            return OperationResult::INTERNAL_ERROR;
    }
}

/// SessionExtension::TableAccess without write access flag.
struct PureTableAccess
{
    std::shared_ptr <Disco::SafeLockGuard> guard_;
    Richard::Table *table_;
};

PureTableAccess GetTableReadOrWriteAccess (const SessionExtension *extension, Richard::AnyDataId id)
{
    if (extension == nullptr)
    {
        return {nullptr, nullptr};
    }

    auto iterator = extension->tableAccesses_.find (id);
    if (iterator == extension->tableAccesses_.end () || !iterator->second.guard_)
    {
        return {nullptr, nullptr};
    }
    else
    {
        return {iterator->second.guard_, iterator->second.tableWeakPointer_};
    }
}

PureTableAccess GetTableWriteAccess (const SessionExtension *extension, Richard::AnyDataId id)
{
    // TODO: Rewrite with less duplication.
    if (extension == nullptr)
    {
        return {nullptr, nullptr};
    }

    auto iterator = extension->tableAccesses_.find (id);
    if (iterator == extension->tableAccesses_.end () || !iterator->second.isWriteAccess_ || !iterator->second.guard_)
    {
        return {nullptr, nullptr};
    }
    else
    {
        return {iterator->second.guard_, iterator->second.tableWeakPointer_};
    }
}

bool EnsureNoTableAccess (const ProcessingContext &context, const SessionExtension *extension,
                          QueryId queryId, ResourceId tableId)
{
    {
        PureTableAccess tableWriteAccess = GetTableWriteAccess (extension, tableId);
        if (tableWriteAccess.guard_ != nullptr)
        {
            SendVoidResult (context, queryId, OperationResult::ALREADY_HAS_WRITE_ACCESS);
            return false;
        }
    }

    {
        PureTableAccess tableReadOrWriteAccess = GetTableWriteAccess (extension, tableId);
        if (tableReadOrWriteAccess.guard_ != nullptr)
        {
            SendVoidResult (context, queryId, OperationResult::ALREADY_HAS_READ_ACCESS);
            return false;
        }
    }

    return true;
}

bool GetTable (const ProcessingContext &context, const SessionExtension *extension,
               QueryId queryId, ResourceId tableId, Richard::Table *&output)
{
    assert (extension);
    assert (context.databaseConduit_);

    Richard::ResultCode databaseResult = context.databaseConduit_->GetTable (
        extension->conduitReadGuard_ == nullptr ?
        extension->conduitWriteGuard_ : extension->conduitReadGuard_,
        tableId, output);

    if (databaseResult == Richard::ResultCode::OK)
    {
        return true;
    }
    else
    {
        SendVoidResult (context, queryId, MapDatabaseResultToOperationResult (databaseResult));
        return false;
    }
}

bool EnsureTableWriteAccess (const ProcessingContext &context, const SessionExtension *extension,
                             QueryId queryId, ResourceId tableId, PureTableAccess &tableAccess)
{
    tableAccess = GetTableWriteAccess (extension, tableId);
    if (tableAccess.guard_ == nullptr)
    {
        SendVoidResult (context, queryId, OperationResult::TABLE_WRITE_ACCESS_REQUIRED);
        return false;
    }
    else
    {
        return true;
    }
}

bool EnsureTableReadOrWriteAccess (const ProcessingContext &context, const SessionExtension *extension,
                                   QueryId queryId, ResourceId tableId, PureTableAccess &tableAccess)
{
    tableAccess = GetTableReadOrWriteAccess (extension, tableId);
    if (tableAccess.guard_ == nullptr)
    {
        SendVoidResult (context, queryId, OperationResult::TABLE_READ_OR_WRITE_ACCESS_REQUIRED);
        return false;
    }
    else
    {
        return true;
    }
}

template <typename Cursor>
struct TransitCursorData
{
    Cursor *cursor_;
    Richard::AnyDataId sourceTableId_ = 0u;
};

TransitCursorData <Richard::TableEditCursor> GetEditCursor (
    const ProcessingContext &context, const SessionExtension *extension, ResourceId cursorId)
{
    if (!extension)
    {
        return {nullptr, 0u};
    }

    auto iterator = extension->editCursors_.find (cursorId);
    if (iterator == extension->editCursors_.end ())
    {
        return {nullptr, 0u};
    }
    else
    {
        return {iterator->second.cursor_.get (), iterator->second.sourceTableId_};
    }
}

TransitCursorData <Richard::TableReadCursor> GetReadOrEditCursor (
    const ProcessingContext &context, const SessionExtension *extension, ResourceId cursorId)
{
    if (!extension)
    {
        return {nullptr, 0u};
    }

    auto iterator = extension->readCursors_.find (cursorId);
    if (iterator == extension->readCursors_.end ())
    {
        TransitCursorData <Richard::TableEditCursor> editCursor = GetEditCursor (context, extension, cursorId);
        if (editCursor.cursor_)
        {
            // It's safe to downcast edit cursor pointer to read cursor pointer.
            return {editCursor.cursor_, editCursor.sourceTableId_};
        }
        else
        {
            return {nullptr, 0u};
        }
    }
    else
    {
        return {iterator->second.cursor_.get (), iterator->second.sourceTableId_};
    }
}

bool UnwrapRowValues (
    const ProcessingContext &context, QueryId queryId,
    std::vector <std::pair <ResourceId, Richard::AnyDataContainer>> &values,
    std::unordered_map <Richard::AnyDataId, Richard::AnyDataContainer> &valuesMap)
{
    for (auto &idValuePair : values)
    {
        if (valuesMap.count (idValuePair.first))
        {
            SendVoidResult (
                context, queryId, Messaging::OperationResult::DUPLICATE_COLUMN_VALUES_IN_INSERTION_REQUEST);
            return false;
        }
        else
        {
            valuesMap[idValuePair.first] = std::move (idValuePair.second);
        }
    }

    return true;
}
}

// TODO: Use ifs with pointer asserts? Like not only assertb (table), but also with if (table) for steel stability.
// TODO: Try to rewrite processors with less duplication.

void ProcessGetTableReadAccessRequest (const ProcessingContext &context,
                                       const TableOperationRequest &message)
{
    using namespace Details;
    assert (context.session_);

    Disco::After (
        &context.session_->Data ().ReadWriteGuard ().Read (),
        [context, request (message)] (auto guard)
        {
            const SessionExtension *extension = nullptr;
            if (ExtractConstSessionExtension (context, request.queryId_, guard, extension))
            {
                if (!EnsureNoTableAccess (context, extension, request.queryId_, request.tableId_))
                {
                    return;
                }

                if (extension == nullptr || (extension->conduitReadGuard_ == nullptr &&
                                             extension->conduitWriteGuard_ == nullptr))
                {
                    SendVoidResult (context, request.queryId_,
                                    OperationResult::CONDUIT_READ_OR_WRITE_ACCESS_REQUIRED);
                }
                else
                {
                    assert (context.databaseConduit_);
                    Richard::Table *table = nullptr;

                    if (GetTable (context, extension, request.queryId_, request.tableId_, table))
                    {
                        assert (table);
                        Disco::After (
                            {Disco::AnyLockPointer (&context.session_->Data ().ReadWriteGuard ().Write ()),
                             Disco::AnyLockPointer (&table->ReadWriteGuard ().Read ())},
                            [context, request, table] (auto guards)
                            {
                                assert (guards.size () == 2u);
                                SessionExtension *extension = nullptr;

                                if (ExtractSessionExtension (context, request.queryId_, guards[0], extension))
                                {
                                    assert (extension);
                                    extension->tableAccesses_[table->GetId ()] =
                                        {guards[1], table, false};
                                    SendVoidResult (context, request.queryId_, OperationResult::OK);
                                }
                            });
                    }
                }
            }
        });
}

void ProcessGetTableWriteAccessRequest (const ProcessingContext &context,
                                        const TableOperationRequest &message)
{
    using namespace Details;
    assert (context.session_);

    Disco::After (
        &context.session_->Data ().ReadWriteGuard ().Read (),
        [context, request (message)] (auto guard)
        {
            const SessionExtension *extension = nullptr;
            if (ExtractConstSessionExtension (context, request.queryId_, guard, extension))
            {
                if (!EnsureNoTableAccess (context, extension, request.queryId_, request.tableId_))
                {
                    return;
                }

                if (extension == nullptr || (extension->conduitReadGuard_ == nullptr &&
                                             extension->conduitWriteGuard_ == nullptr))
                {
                    SendVoidResult (context, request.queryId_,
                                    OperationResult::CONDUIT_READ_OR_WRITE_ACCESS_REQUIRED);
                }
                else
                {
                    assert (context.databaseConduit_);
                    Richard::Table *table = nullptr;

                    if (GetTable (context, extension, request.queryId_, request.tableId_, table))
                    {
                        assert (table);
                        Disco::After (
                            {Disco::AnyLockPointer (&context.session_->Data ().ReadWriteGuard ().Write ()),
                             Disco::AnyLockPointer (&table->ReadWriteGuard ().Write ())},
                            [context, request, table] (auto guards)
                            {
                                assert (guards.size () == 2u);
                                SessionExtension *extension = nullptr;

                                if (ExtractSessionExtension (context, request.queryId_, guards[0], extension))
                                {
                                    assert (extension);
                                    extension->tableAccesses_[table->GetId ()] =
                                        {guards[1], table, true};
                                    SendVoidResult (context, request.queryId_, OperationResult::OK);
                                }
                            });
                    }
                }
            }
        });
}

void ProcessCloseTableReadAccessRequest (const ProcessingContext &context,
                                         const TableOperationRequest &message)
{
    using namespace Details;
    assert (context.session_);

    Disco::After (
        &context.session_->Data ().ReadWriteGuard ().Write (),
        [context, request (message)] (auto guard)
        {
            SessionExtension *extension = nullptr;
            if (ExtractSessionExtension (context, request.queryId_, guard, extension))
            {
                assert (extension);
                auto iterator = extension->tableAccesses_.find (request.tableId_);

                if (iterator == extension->tableAccesses_.end () ||
                    (iterator->second.guard_ && iterator->second.isWriteAccess_))
                {
                    SendVoidResult (context, request.queryId_, OperationResult::TABLE_READ_ACCESS_REQUIRED);
                }
                else
                {
                    extension->tableAccesses_.erase (iterator);
                    SendVoidResult (context, request.queryId_, OperationResult::OK);
                }
            }
        });
}

void ProcessCloseTableWriteAccessRequest (const ProcessingContext &context,
                                          const TableOperationRequest &message)
{
    using namespace Details;
    assert (context.session_);

    Disco::After (
        &context.session_->Data ().ReadWriteGuard ().Write (),
        [context, request (message)] (auto guard)
        {
            SessionExtension *extension = nullptr;
            if (ExtractSessionExtension (context, request.queryId_, guard, extension))
            {
                assert (extension);
                auto iterator = extension->tableAccesses_.find (request.tableId_);

                if (iterator == extension->tableAccesses_.end () ||
                    (iterator->second.guard_ && !iterator->second.isWriteAccess_))
                {
                    SendVoidResult (context, request.queryId_, OperationResult::TABLE_WRITE_ACCESS_REQUIRED);
                }
                else
                {
                    extension->tableAccesses_.erase (iterator);
                    SendVoidResult (context, request.queryId_, OperationResult::OK);
                }
            }
        });
}

void ProcessGetTableNameRequest (const ProcessingContext &context,
                                 const TableOperationRequest &message)
{
    using namespace Details;
    assert (context.session_);

    Disco::After (
        &context.session_->Data ().ReadWriteGuard ().Read (),
        [context, request (message)] (auto guard)
        {
            const SessionExtension *extension = nullptr;
            if (ExtractConstSessionExtension (context, request.queryId_, guard, extension))
            {
                PureTableAccess tableAccess {};
                if (EnsureTableReadOrWriteAccess (
                    context, extension, request.queryId_, request.tableId_, tableAccess))
                {
                    assert (tableAccess.table_);
                    GetTableNameResponse response {};
                    response.queryId_ = request.queryId_;

                    Richard::ResultCode result = tableAccess.table_->GetName (
                        tableAccess.guard_, response.tableName_);

                    if (result == Richard::ResultCode::OK)
                    {
                        response.Write (Messaging::Message::GET_TABLE_NAME_RESPONSE, context.session_);
                    }
                    else
                    {
                        SendVoidResult (context, request.queryId_, MapDatabaseResultToOperationResult (result));
                    }
                }
            }
        });
}

void ProcessCreateReadCursorRequest (const ProcessingContext &context,
                                     const TablePartOperationRequest &message)
{
    using namespace Details;
    assert (context.session_);

    Disco::After (
        &context.session_->Data ().ReadWriteGuard ().Write (),
        [context, request (message)] (auto guard)
        {
            SessionExtension *extension = nullptr;
            if (ExtractSessionExtension (context, request.queryId_, guard, extension))
            {
                assert (extension);
                PureTableAccess tableAccess {};

                if (EnsureTableReadOrWriteAccess (
                    context, extension, request.queryId_, request.tableId_, tableAccess))
                {
                    assert (tableAccess.table_);
                    CreateOperationResultResponse response {};
                    response.queryId_ = request.queryId_;

                    Richard::TableReadCursor *cursor = nullptr;
                    Richard::ResultCode result = tableAccess.table_->CreateReadCursor (
                        tableAccess.guard_, request.partId_, cursor);

                    if (result == Richard::ResultCode::OK)
                    {
                        assert (cursor);
                        Richard::AnyDataId cursorId = extension->nextCursorId_++;
                        SessionExtension::CursorData <Richard::TableReadCursor> cursorData {};
                        cursorData.cursor_.reset (cursor);
                        cursorData.sourceTableId_ = request.tableId_;

                        auto emplaceResult = extension->readCursors_.emplace (cursorId, std::move (cursorData));
                        // TODO: Better check later.
                        assert (emplaceResult.second);

                        response.resourceId_ = cursorId;
                        response.Write (Messaging::Message::CREATE_OPERATION_RESULT_RESPONSE,
                                        context.session_);
                    }
                    else
                    {
                        SendVoidResult (context, request.queryId_, MapDatabaseResultToOperationResult (result));
                    }
                }
            }
        });
}

void ProcessGetColumnsIdsRequest (const ProcessingContext &context,
                                  const TableOperationRequest &message)
{
    using namespace Details;
    assert (context.session_);

    Disco::After (
        &context.session_->Data ().ReadWriteGuard ().Read (),
        [context, request (message)] (auto guard)
        {
            const SessionExtension *extension = nullptr;
            if (ExtractConstSessionExtension (context, request.queryId_, guard, extension))
            {
                PureTableAccess tableAccess {};
                if (EnsureTableReadOrWriteAccess (
                    context, extension, request.queryId_, request.tableId_, tableAccess))
                {
                    assert (tableAccess.table_);
                    IdsResponse response {};
                    response.queryId_ = request.queryId_;

                    Richard::ResultCode result = tableAccess.table_->GetColumnsIds (
                        tableAccess.guard_, response.ids_);

                    if (result == Richard::ResultCode::OK)
                    {
                        response.Write (Messaging::Message::RESOURCE_IDS_RESPONSE, context.session_);
                    }
                    else
                    {
                        SendVoidResult (context, request.queryId_, MapDatabaseResultToOperationResult (result));
                    }
                }
            }
        });
}

void ProcessGetColumnInfoRequest (const ProcessingContext &context,
                                  const TablePartOperationRequest &message)
{
    using namespace Details;
    assert (context.session_);

    Disco::After (
        &context.session_->Data ().ReadWriteGuard ().Read (),
        [context, request (message)] (auto guard)
        {
            const SessionExtension *extension = nullptr;
            if (ExtractConstSessionExtension (context, request.queryId_, guard, extension))
            {
                PureTableAccess tableAccess {};
                if (EnsureTableReadOrWriteAccess (
                    context, extension, request.queryId_, request.tableId_, tableAccess))
                {
                    assert (tableAccess.table_);
                    ColumnInfoResponse response {};
                    response.queryId_ = request.queryId_;

                    Richard::ColumnInfo info {};
                    Richard::ResultCode result = tableAccess.table_->GetColumnInfo (
                        tableAccess.guard_, request.partId_, info);

                    if (result == Richard::ResultCode::OK)
                    {
                        response.dataType_ = info.dataType_;
                        response.name_ = info.name_;
                        response.Write (Messaging::Message::GET_COLUMN_INFO_RESPONSE, context.session_);
                    }
                    else
                    {
                        SendVoidResult (context, request.queryId_, MapDatabaseResultToOperationResult (result));
                    }
                }
            }
        });
}

void ProcessGetIndicesIdsRequest (const ProcessingContext &context,
                                  const TableOperationRequest &message)
{
    using namespace Details;
    assert (context.session_);

    Disco::After (
        &context.session_->Data ().ReadWriteGuard ().Read (),
        [context, request (message)] (auto guard)
        {
            const SessionExtension *extension = nullptr;
            if (ExtractConstSessionExtension (context, request.queryId_, guard, extension))
            {
                PureTableAccess tableAccess {};
                if (EnsureTableReadOrWriteAccess (
                    context, extension, request.queryId_, request.tableId_, tableAccess))
                {
                    assert (tableAccess.table_);
                    IdsResponse response {};
                    response.queryId_ = request.queryId_;

                    Richard::ResultCode result = tableAccess.table_->GetIndicesIds (
                        tableAccess.guard_, response.ids_);

                    if (result == Richard::ResultCode::OK)
                    {
                        response.Write (Messaging::Message::RESOURCE_IDS_RESPONSE, context.session_);
                    }
                    else
                    {
                        SendVoidResult (context, request.queryId_, MapDatabaseResultToOperationResult (result));
                    }
                }
            }
        });
}

void ProcessGetIndexInfoRequest (const ProcessingContext &context,
                                 const TablePartOperationRequest &message)
{
    using namespace Details;
    assert (context.session_);

    Disco::After (
        &context.session_->Data ().ReadWriteGuard ().Read (),
        [context, request (message)] (auto guard)
        {
            const SessionExtension *extension = nullptr;
            if (ExtractConstSessionExtension (context, request.queryId_, guard, extension))
            {
                PureTableAccess tableAccess {};
                if (EnsureTableReadOrWriteAccess (
                    context, extension, request.queryId_, request.tableId_, tableAccess))
                {
                    assert (tableAccess.table_);
                    IndexInfoResponse response {};
                    response.queryId_ = request.queryId_;

                    Richard::IndexInfo info {};
                    Richard::ResultCode result = tableAccess.table_->GetIndexInfo (
                        tableAccess.guard_, request.partId_, info);

                    if (result == Richard::ResultCode::OK)
                    {
                        response.name_ = info.name_;
                        response.columns_ = info.columns_;
                        response.Write (Messaging::Message::GET_INDEX_INFO_RESPONSE, context.session_);
                    }
                    else
                    {
                        SendVoidResult (context, request.queryId_, MapDatabaseResultToOperationResult (result));
                    }
                }
            }
        });
}

void ProcessSetTableNameRequest (const ProcessingContext &context,
                                 const SetTableNameRequest &message)
{
    using namespace Details;
    assert (context.session_);

    Disco::After (
        &context.session_->Data ().ReadWriteGuard ().Read (),
        [context, request (message)] (auto guard)
        {
            const SessionExtension *extension = nullptr;
            if (ExtractConstSessionExtension (context, request.queryId_, guard, extension))
            {
                PureTableAccess tableAccess {};
                if (EnsureTableWriteAccess (context, extension, request.queryId_, request.tableId_, tableAccess))
                {
                    assert (tableAccess.table_);
                    Richard::ResultCode result = tableAccess.table_->SetName (tableAccess.guard_, request.newName_);
                    SendVoidResult (context, request.queryId_, MapDatabaseResultToOperationResult (result));
                }
            }
        });
}

void ProcessCreateEditCursorRequest (const ProcessingContext &context,
                                     const TablePartOperationRequest &message)
{
    using namespace Details;
    assert (context.session_);

    Disco::After (
        &context.session_->Data ().ReadWriteGuard ().Write (),
        [context, request (message)] (auto guard)
        {
            SessionExtension *extension = nullptr;
            if (ExtractSessionExtension (context, request.queryId_, guard, extension))
            {
                assert (extension);
                PureTableAccess tableAccess {};

                if (EnsureTableWriteAccess (context, extension, request.queryId_, request.tableId_, tableAccess))
                {
                    assert (tableAccess.table_);
                    CreateOperationResultResponse response {};
                    response.queryId_ = request.queryId_;

                    Richard::TableEditCursor *cursor = nullptr;
                    Richard::ResultCode result = tableAccess.table_->CreateEditCursor (
                        tableAccess.guard_, request.partId_, cursor);

                    if (result == Richard::ResultCode::OK)
                    {
                        assert (cursor);
                        Richard::AnyDataId cursorId = extension->nextCursorId_++;
                        SessionExtension::CursorData <Richard::TableEditCursor> cursorData {};
                        cursorData.cursor_.reset (cursor);
                        cursorData.sourceTableId_ = request.tableId_;

                        auto emplaceResult = extension->editCursors_.emplace (cursorId, std::move (cursorData));
                        // TODO: Better check later.
                        assert (emplaceResult.second);

                        response.resourceId_ = cursorId;
                        response.Write (Messaging::Message::CREATE_OPERATION_RESULT_RESPONSE,
                                        context.session_);
                    }
                    else
                    {
                        SendVoidResult (context, request.queryId_, MapDatabaseResultToOperationResult (result));
                    }
                }
            }
        });
}

void ProcessAddColumnRequest (const ProcessingContext &context,
                              const AddColumnRequest &message)
{
    using namespace Details;
    assert (context.session_);

    Disco::After (
        &context.session_->Data ().ReadWriteGuard ().Read (),
        [context, request (message)] (auto guard)
        {
            const SessionExtension *extension = nullptr;
            if (ExtractConstSessionExtension (context, request.queryId_, guard, extension))
            {
                PureTableAccess tableAccess {};
                if (EnsureTableWriteAccess (context, extension, request.queryId_, request.tableId_, tableAccess))
                {
                    assert (tableAccess.table_);
                    CreateOperationResultResponse response {};
                    response.queryId_ = request.queryId_;

                    Richard::ResultCode result = tableAccess.table_->AddColumn (
                        tableAccess.guard_, {0u, request.dataType_, request.name_}, response.resourceId_);

                    if (result == Richard::ResultCode::OK)
                    {
                        response.Write (
                            Messaging::Message::CREATE_OPERATION_RESULT_RESPONSE, context.session_);
                    }
                    else
                    {
                        SendVoidResult (context, request.queryId_, MapDatabaseResultToOperationResult (result));
                    }
                }
            }
        });
}

void ProcessRemoveColumnRequest (const ProcessingContext &context,
                                 const TablePartOperationRequest &message)
{
    using namespace Details;
    assert (context.session_);

    Disco::After (
        &context.session_->Data ().ReadWriteGuard ().Read (),
        [context, request (message)] (auto guard)
        {
            const SessionExtension *extension = nullptr;
            if (ExtractConstSessionExtension (context, request.queryId_, guard, extension))
            {
                PureTableAccess tableAccess {};
                if (EnsureTableWriteAccess (context, extension, request.queryId_, request.tableId_, tableAccess))
                {
                    assert (tableAccess.table_);
                    Richard::ResultCode result = tableAccess.table_->RemoveColumn (
                        tableAccess.guard_, request.partId_);
                    SendVoidResult (context, request.queryId_, MapDatabaseResultToOperationResult (result));
                }
            }
        });
}

void ProcessAddIndexRequest (const ProcessingContext &context,
                             const AddIndexRequest &message)
{
    using namespace Details;
    assert (context.session_);

    Disco::After (
        &context.session_->Data ().ReadWriteGuard ().Read (),
        [context, request (message)] (auto guard)
        {
            const SessionExtension *extension = nullptr;
            if (ExtractConstSessionExtension (context, request.queryId_, guard, extension))
            {
                PureTableAccess tableAccess {};
                if (EnsureTableWriteAccess (context, extension, request.queryId_, request.tableId_, tableAccess))
                {
                    assert (tableAccess.table_);
                    CreateOperationResultResponse response {};
                    response.queryId_ = request.queryId_;

                    Richard::ResultCode result = tableAccess.table_->AddIndex (
                        tableAccess.guard_, {0u, request.name_, request.columns_}, response.resourceId_);

                    if (result == Richard::ResultCode::OK)
                    {
                        response.Write (
                            Messaging::Message::CREATE_OPERATION_RESULT_RESPONSE, context.session_);
                    }
                    else
                    {
                        SendVoidResult (context, request.queryId_, MapDatabaseResultToOperationResult (result));
                    }
                }
            }
        });
}

void ProcessRemoveIndexRequest (const ProcessingContext &context,
                                const TablePartOperationRequest &message)
{
    using namespace Details;
    assert (context.session_);

    Disco::After (
        &context.session_->Data ().ReadWriteGuard ().Read (),
        [context, request (message)] (auto guard)
        {
            const SessionExtension *extension = nullptr;
            if (ExtractConstSessionExtension (context, request.queryId_, guard, extension))
            {
                PureTableAccess tableAccess {};
                if (EnsureTableWriteAccess (context, extension, request.queryId_, request.tableId_, tableAccess))
                {
                    assert (tableAccess.table_);
                    Richard::ResultCode result = tableAccess.table_->RemoveIndex (tableAccess.guard_, request.partId_);
                    SendVoidResult (context, request.queryId_, MapDatabaseResultToOperationResult (result));
                }
            }
        });
}

void ProcessAddRowRequest (const ProcessingContext &context, AddRowRequest &message)
{
    using namespace Details;
    assert (context.session_);

    Disco::After (
        &context.session_->Data ().ReadWriteGuard ().Read (),
        // Request is captured using shared pointer because std::function requires all captures to be copyable,
        // but it's impossible to copy this request because of Richard::AnyDataContainer.
        [context, request (std::make_shared <AddRowRequest> (std::move (message)))] (auto guard) mutable
        {
            const SessionExtension *extension = nullptr;
            if (ExtractConstSessionExtension (context, request->queryId_, guard, extension))
            {
                PureTableAccess tableAccess {};
                if (EnsureTableWriteAccess (context, extension, request->queryId_, request->tableId_, tableAccess))
                {
                    assert (tableAccess.table_);
                    std::unordered_map <Richard::AnyDataId, Richard::AnyDataContainer> valuesMap;

                    if (UnwrapRowValues (context, request->queryId_, request->values_, valuesMap))
                    {
                        Richard::ResultCode result = tableAccess.table_->InsertRow (tableAccess.guard_, valuesMap);
                        SendVoidResult (context, request->queryId_, MapDatabaseResultToOperationResult (result));
                    }
                }
            }
        });
}

void ProcessCursorAdvanceRequest (const ProcessingContext &context,
                                  const CursorAdvanceRequest &message)
{
    using namespace Details;
    assert (context.session_);

    Disco::After (
        &context.session_->Data ().ReadWriteGuard ().Read (),
        // Request is captured using shared pointer because std::function requires all captures to be copyable,
        // but it's impossible to copy this request because of Richard::AnyDataContainer.
        [context, request (message)] (auto guard) mutable
        {
            const SessionExtension *extension = nullptr;
            if (ExtractConstSessionExtension (context, request.queryId_, guard, extension))
            {
                TransitCursorData <Richard::TableReadCursor> cursorData =
                    GetReadOrEditCursor (context, extension, request.cursorId_);

                if (!cursorData.cursor_)
                {
                    SendVoidResult (context, request.queryId_,
                                    Messaging::OperationResult::CURSOR_WITH_GIVEN_ID_NOT_FOUND);
                    return;
                }

                PureTableAccess tableAccess {};
                if (EnsureTableReadOrWriteAccess (
                    context, extension, request.queryId_, cursorData.sourceTableId_, tableAccess))
                {
                    Richard::ResultCode result = cursorData.cursor_->Advance (tableAccess.guard_, request.step_);
                    SendVoidResult (context, request.queryId_, MapDatabaseResultToOperationResult (result));
                }
            }
        });
}

void ProcessCursorGetRequest (const ProcessingContext &context,
                              const CursorGetRequest &message)
{
    using namespace Details;
    assert (context.session_);

    Disco::After (
        &context.session_->Data ().ReadWriteGuard ().Read (),
        // Request is captured using shared pointer because std::function requires all captures to be copyable,
        // but it's impossible to copy this request because of Richard::AnyDataContainer.
        [context, request (message)] (auto guard) mutable
        {
            const SessionExtension *extension = nullptr;
            if (ExtractConstSessionExtension (context, request.queryId_, guard, extension))
            {
                TransitCursorData <Richard::TableReadCursor> cursorData =
                    GetReadOrEditCursor (context, extension, request.cursorId_);

                if (!cursorData.cursor_)
                {
                    SendVoidResult (context, request.queryId_,
                                    Messaging::OperationResult::CURSOR_WITH_GIVEN_ID_NOT_FOUND);
                    return;
                }

                PureTableAccess tableAccess {};
                if (EnsureTableReadOrWriteAccess (
                    context, extension, request.queryId_, cursorData.sourceTableId_, tableAccess))
                {
                    CursorGetResponse response {};
                    response.queryId_ = request.queryId_;

                    const Richard::AnyDataContainer *container = nullptr;
                    Richard::ResultCode result = cursorData.cursor_->Get (
                        tableAccess.guard_, request.columnId_, container);

                    if (result == Richard::ResultCode::OK)
                    {
                        if (container)
                        {
                            response.value_.CopyFrom (*container);
                            response.Write (Messaging::Message::CURSOR_GET_RESPONSE, context.session_);
                        }
                        else
                        {
                            SendVoidResult (context, request.queryId_,
                                            Messaging::OperationResult::NULL_COLUMN_VALUE);
                        }
                    }
                    else
                    {
                        SendVoidResult (context, request.queryId_, MapDatabaseResultToOperationResult (result));
                    }
                }
            }
        });
}

void ProcessCursorUpdateRequest (const ProcessingContext &context,
                                 CursorUpdateRequest &message)
{
    using namespace Details;
    assert (context.session_);

    Disco::After (
        &context.session_->Data ().ReadWriteGuard ().Read (),
        // Request is captured using shared pointer because std::function requires all captures to be copyable,
        // but it's impossible to copy this request because of Richard::AnyDataContainer.
        [context, request (std::make_shared <CursorUpdateRequest> (std::move (message)))] (auto guard) mutable
        {
            const SessionExtension *extension = nullptr;
            if (ExtractConstSessionExtension (context, request->queryId_, guard, extension))
            {
                TransitCursorData <Richard::TableEditCursor> cursorData =
                    GetEditCursor (context, extension, request->cursorId_);

                if (!cursorData.cursor_)
                {
                    SendVoidResult (context, request->queryId_,
                                    Messaging::OperationResult::CURSOR_WITH_GIVEN_ID_NOT_FOUND);
                    return;
                }

                PureTableAccess tableAccess {};
                if (EnsureTableWriteAccess (
                    context, extension, request->queryId_, cursorData.sourceTableId_, tableAccess))
                {
                    std::unordered_map <Richard::AnyDataId, Richard::AnyDataContainer> valuesMap;
                    if (UnwrapRowValues (context, request->queryId_, request->values_, valuesMap))
                    {
                        Richard::ResultCode result = cursorData.cursor_->Update (tableAccess.guard_, valuesMap);
                        SendVoidResult (context, request->queryId_, MapDatabaseResultToOperationResult (result));
                    }
                }
            }
        });
}

void ProcessCursorDeleteRequest (const ProcessingContext &context,
                                 const CursorVoidActionRequest &message)
{
    using namespace Details;
    assert (context.session_);

    Disco::After (
        &context.session_->Data ().ReadWriteGuard ().Read (),
        // Request is captured using shared pointer because std::function requires all captures to be copyable,
        // but it's impossible to copy this request because of Richard::AnyDataContainer.
        [context, request (message)] (auto guard) mutable
        {
            const SessionExtension *extension = nullptr;
            if (ExtractConstSessionExtension (context, request.queryId_, guard, extension))
            {
                TransitCursorData <Richard::TableEditCursor> cursorData =
                    GetEditCursor (context, extension, request.cursorId_);

                if (!cursorData.cursor_)
                {
                    SendVoidResult (context, request.queryId_,
                                    Messaging::OperationResult::CURSOR_WITH_GIVEN_ID_NOT_FOUND);
                    return;
                }

                PureTableAccess tableAccess {};
                if (EnsureTableWriteAccess (
                    context, extension, request.queryId_, cursorData.sourceTableId_, tableAccess))
                {
                    Richard::ResultCode result = cursorData.cursor_->DeleteCurrent (tableAccess.guard_);
                    SendVoidResult (context, request.queryId_, MapDatabaseResultToOperationResult (result));
                }
            }
        });
}

void ProcessCloseCursorRequest (const ProcessingContext &context,
                                const CursorVoidActionRequest &message)
{
    using namespace Details;
    assert (context.session_);

    Disco::After (
        &context.session_->Data ().ReadWriteGuard ().Write (),
        // Request is captured using shared pointer because std::function requires all captures to be copyable,
        // but it's impossible to copy this request because of Richard::AnyDataContainer.
        [context, request (message)] (auto guard) mutable
        {
            SessionExtension *extension = nullptr;
            if (ExtractSessionExtension (context, request.queryId_, guard, extension))
            {
                assert (extension);
                {
                    auto iterator = extension->readCursors_.find (request.cursorId_);
                    if (iterator != extension->readCursors_.end ())
                    {
                        // TODO: Check for collision in other places too.
                        assert (extension->editCursors_.find (request.cursorId_) == extension->editCursors_.end ());
                        extension->readCursors_.erase (iterator);
                        SendVoidResult (context, request.queryId_, Messaging::OperationResult::OK);
                        return;
                    }
                }

                {
                    auto iterator = extension->editCursors_.find (request.cursorId_);
                    if (iterator != extension->editCursors_.end ())
                    {
                        extension->editCursors_.erase (iterator);
                        SendVoidResult (context, request.queryId_, Messaging::OperationResult::OK);
                        return;
                    }
                }

                SendVoidResult (context, request.queryId_,
                                Messaging::OperationResult::CURSOR_WITH_GIVEN_ID_NOT_FOUND);
            }
        });
}

void ProcessGetConduitReadAccessRequest (const ProcessingContext &context,
                                         const ConduitVoidActionRequest &message)
{
    using namespace Details;
    assert (context.session_);

    Disco::After (
        &context.session_->Data ().ReadWriteGuard ().Read (),
        [context, request (message)] (auto guard)
        {
            const SessionExtension *extension = nullptr;
            if (ExtractConstSessionExtension (context, request.queryId_, guard, extension))
            {
                if (extension != nullptr && extension->conduitWriteGuard_ != nullptr)
                {
                    SendVoidResult (context, request.queryId_, OperationResult::ALREADY_HAS_WRITE_ACCESS);
                }
                else if (extension == nullptr || extension->conduitReadGuard_ == nullptr)
                {
                    assert (context.databaseConduit_);
                    Disco::After (
                        {Disco::AnyLockPointer (&context.session_->Data ().ReadWriteGuard ().Write ()),
                         Disco::AnyLockPointer (&context.databaseConduit_->ReadWriteGuard ().Read ())},
                        [context, request] (auto guards)
                        {
                            assert (guards.size () == 2u);
                            SessionExtension *extension = nullptr;

                            if (ExtractSessionExtension (context, request.queryId_, guards[0], extension))
                            {
                                assert (extension);
                                extension->conduitReadGuard_ = guards[1];
                                SendVoidResult (context, request.queryId_, OperationResult::OK);
                            }
                        });
                }
                else
                {
                    SendVoidResult (context, request.queryId_, OperationResult::ALREADY_HAS_READ_ACCESS);
                }
            }
        });
}

void ProcessGetConduitWriteAccessRequest (const ProcessingContext &context,
                                          const ConduitVoidActionRequest &message)
{
    using namespace Details;
    assert (context.session_);

    Disco::After (
        &context.session_->Data ().ReadWriteGuard ().Read (),
        [context, request (message)] (auto guard)
        {
            const SessionExtension *extension = nullptr;
            if (ExtractConstSessionExtension (context, request.queryId_, guard, extension))
            {
                if (extension != nullptr && extension->conduitReadGuard_ != nullptr)
                {
                    SendVoidResult (context, request.queryId_, OperationResult::ALREADY_HAS_READ_ACCESS);
                }
                else if (extension == nullptr || extension->conduitWriteGuard_ == nullptr)
                {
                    assert (context.databaseConduit_);
                    Disco::After (
                        {Disco::AnyLockPointer (&context.session_->Data ().ReadWriteGuard ().Write ()),
                         Disco::AnyLockPointer (&context.databaseConduit_->ReadWriteGuard ().Write ())},
                        [context, request] (auto guards)
                        {
                            assert (guards.size () == 2u);
                            SessionExtension *extension = nullptr;

                            if (ExtractSessionExtension (context, request.queryId_, guards[0], extension))
                            {
                                assert (extension);
                                extension->conduitWriteGuard_ = guards[1];
                                SendVoidResult (context, request.queryId_, OperationResult::OK);
                            }
                        });
                }
                else
                {
                    SendVoidResult (context, request.queryId_, OperationResult::ALREADY_HAS_WRITE_ACCESS);
                }
            }
        });
}

void ProcessCloseConduitReadAccessRequest (const ProcessingContext &context,
                                           const ConduitVoidActionRequest &message)
{
    using namespace Details;
    assert (context.session_);

    Disco::After (
        &context.session_->Data ().ReadWriteGuard ().Write (),
        [context, request (message)] (auto guard)
        {
            SessionExtension *extension = nullptr;
            if (ExtractSessionExtension (context, request.queryId_, guard, extension))
            {
                assert (extension);
                if (extension->conduitReadGuard_ == nullptr)
                {
                    SendVoidResult (context, request.queryId_, OperationResult::CONDUIT_READ_ACCESS_REQUIRED);
                }
                else
                {
                    extension->conduitReadGuard_ = nullptr;
                    SendVoidResult (context, request.queryId_, OperationResult::OK);
                }
            }
        });
}

void ProcessCloseConduitWriteAccessRequest (const ProcessingContext &context,
                                            const ConduitVoidActionRequest &message)
{
    using namespace Details;
    assert (context.session_);

    Disco::After (
        &context.session_->Data ().ReadWriteGuard ().Write (),
        [context, request (message)] (auto guard)
        {
            SessionExtension *extension = nullptr;
            if (ExtractSessionExtension (context, request.queryId_, guard, extension))
            {
                assert (extension);
                if (extension->conduitWriteGuard_ == nullptr)
                {
                    SendVoidResult (context, request.queryId_, OperationResult::CONDUIT_WRITE_ACCESS_REQUIRED);
                }
                else
                {
                    extension->conduitWriteGuard_ = nullptr;
                    SendVoidResult (context, request.queryId_, OperationResult::OK);
                }
            }
        });
}

void ProcessGetTableIdsRequest (const ProcessingContext &context,
                                const ConduitVoidActionRequest &message)
{
    using namespace Details;
    assert (context.session_);

    Disco::After (
        &context.session_->Data ().ReadWriteGuard ().Read (),
        [context, request (message)] (auto guard)
        {
            const SessionExtension *extension = nullptr;
            if (ExtractConstSessionExtension (context, request.queryId_, guard, extension))
            {
                if (extension == nullptr || (extension->conduitReadGuard_ == nullptr &&
                                             extension->conduitWriteGuard_ == nullptr))
                {
                    SendVoidResult (context, request.queryId_,
                                    OperationResult::CONDUIT_READ_OR_WRITE_ACCESS_REQUIRED);
                }
                else
                {
                    assert (context.databaseConduit_);
                    IdsResponse response {};
                    response.queryId_ = request.queryId_;

                    Richard::ResultCode resultCode = context.databaseConduit_->GetTableIds (
                        extension->conduitReadGuard_ == nullptr ?
                        extension->conduitWriteGuard_ : extension->conduitReadGuard_,
                        response.ids_);

                    if (resultCode == Richard::ResultCode::OK)
                    {
                        response.Write (Messaging::Message::RESOURCE_IDS_RESPONSE, context.session_);
                    }
                    else
                    {
                        SendVoidResult (context, request.queryId_,
                                        MapDatabaseResultToOperationResult (resultCode));
                    }
                }
            }
        });
}

void ProcessAddTableRequest (const ProcessingContext &context,
                             const AddTableRequest &message)
{
    using namespace Details;
    assert (context.session_);

    Disco::After (
        &context.session_->Data ().ReadWriteGuard ().Read (),
        [context, request (message)] (auto guard)
        {
            const SessionExtension *extension = nullptr;
            if (ExtractConstSessionExtension (context, request.queryId_, guard, extension))
            {
                if (extension == nullptr || extension->conduitWriteGuard_ == nullptr)
                {
                    SendVoidResult (context, request.queryId_, OperationResult::CONDUIT_WRITE_ACCESS_REQUIRED);
                }
                else
                {
                    assert (context.databaseConduit_);
                    CreateOperationResultResponse response {};
                    response.queryId_ = request.queryId_;

                    Richard::ResultCode resultCode = context.databaseConduit_->AddTable (
                        extension->conduitWriteGuard_, request.tableName_, response.resourceId_);

                    if (resultCode == Richard::ResultCode::OK)
                    {
                        response.Write (
                            Messaging::Message::CREATE_OPERATION_RESULT_RESPONSE, context.session_);
                    }
                    else
                    {
                        SendVoidResult (context, request.queryId_,
                                        MapDatabaseResultToOperationResult (resultCode));
                    }
                }
            }
        });
}

void ProcessRemoveTableRequest (const ProcessingContext &context,
                                const TableOperationRequest &message)
{
    using namespace Details;
    assert (context.session_);

    Disco::After (
        &context.session_->Data ().ReadWriteGuard ().Read (),
        [context, request (message)] (auto guard)
        {
            const SessionExtension *extension = nullptr;
            if (ExtractConstSessionExtension (context, request.queryId_, guard, extension))
            {
                if (extension == nullptr || extension->conduitWriteGuard_ == nullptr)
                {
                    SendVoidResult (context, request.queryId_, OperationResult::CONDUIT_WRITE_ACCESS_REQUIRED);
                }
                else
                {
                    assert (context.databaseConduit_);
                    PureTableAccess tableAccess = GetTableWriteAccess (extension, request.tableId_);
                    if (tableAccess.guard_ == nullptr)
                    {
                        SendVoidResult (context, request.queryId_, OperationResult::TABLE_WRITE_ACCESS_REQUIRED);
                    }
                    else
                    {
                        Richard::ResultCode resultCode = context.databaseConduit_->RemoveTable (
                            extension->conduitWriteGuard_, tableAccess.guard_, request.tableId_);

                        SendVoidResult (context, request.queryId_,
                                        MapDatabaseResultToOperationResult (resultCode));
                    }
                }
            }
        });
}
}