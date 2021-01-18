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

std::pair <std::shared_ptr <Disco::SafeLockGuard>, Richard::Table *> GetTableReadOrWriteAccessInfo (
    const SessionExtension *extension, Richard::AnyDataId id)
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

std::pair <std::shared_ptr <Disco::SafeLockGuard>, Richard::Table *> GetTableWriteAccessInfo (
    const SessionExtension *extension, Richard::AnyDataId id)
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
}

void ProcessGetTableReadAccessRequest (const ProcessingContext &context,
                                       const TableOperationRequest &message)
{
    // TODO: Stub, implement real logic.
    VoidOperationResultResponse response
        {message.queryId_, OperationResult::OK};
    response.Write (Message::VOID_OPERATION_RESULT_RESPONSE, context.session_);
}

void ProcessGetTableWriteAccessRequest (const ProcessingContext &context,
                                        const TableOperationRequest &message)
{
    // TODO: Stub, implement real logic.
    VoidOperationResultResponse response
        {message.queryId_, OperationResult::OK};
    response.Write (Message::VOID_OPERATION_RESULT_RESPONSE, context.session_);
}

void ProcessCloseTableReadAccessRequest (const ProcessingContext &context,
                                         const TableOperationRequest &message)
{
    // TODO: Stub, implement real logic.
    VoidOperationResultResponse response
        {message.queryId_, OperationResult::OK};
    response.Write (Message::VOID_OPERATION_RESULT_RESPONSE, context.session_);
}

void ProcessCloseTableWriteAccessRequest (const ProcessingContext &context,
                                          const TableOperationRequest &message)
{
    // TODO: Stub, implement real logic.
    VoidOperationResultResponse response
        {message.queryId_, OperationResult::OK};
    response.Write (Message::VOID_OPERATION_RESULT_RESPONSE, context.session_);
}

void ProcessGetTableNameRequest (const ProcessingContext &context,
                                 const TableOperationRequest &message)
{
    // TODO: Stub, implement real logic.
    VoidOperationResultResponse response
        {message.queryId_, OperationResult::OK};
    response.Write (Message::VOID_OPERATION_RESULT_RESPONSE, context.session_);
}

void ProcessCreateReadCursorRequest (const ProcessingContext &context,
                                     const TablePartOperationRequest &message)
{
    // TODO: Stub, implement real logic.
    VoidOperationResultResponse response
        {message.queryId_, OperationResult::OK};
    response.Write (Message::VOID_OPERATION_RESULT_RESPONSE, context.session_);
}

void ProcessGetColumnsIdsRequest (const ProcessingContext &context,
                                  const TableOperationRequest &message)
{
    // TODO: Stub, implement real logic.
    VoidOperationResultResponse response
        {message.queryId_, OperationResult::OK};
    response.Write (Message::VOID_OPERATION_RESULT_RESPONSE, context.session_);
}

void ProcessGetColumnInfoRequest (const ProcessingContext &context,
                                  const TablePartOperationRequest &message)
{
    // TODO: Stub, implement real logic.
    VoidOperationResultResponse response
        {message.queryId_, OperationResult::OK};
    response.Write (Message::VOID_OPERATION_RESULT_RESPONSE, context.session_);
}

void ProcessGetIndicesIdsRequest (const ProcessingContext &context,
                                  const TableOperationRequest &message)
{
    // TODO: Stub, implement real logic.
    VoidOperationResultResponse response
        {message.queryId_, OperationResult::OK};
    response.Write (Message::VOID_OPERATION_RESULT_RESPONSE, context.session_);
}

void ProcessGetIndexInfoRequest (const ProcessingContext &context,
                                 const TablePartOperationRequest &message)
{
    // TODO: Stub, implement real logic.
    VoidOperationResultResponse response
        {message.queryId_, OperationResult::OK};
    response.Write (Message::VOID_OPERATION_RESULT_RESPONSE, context.session_);
}

void ProcessSetTableNameRequest (const ProcessingContext &context,
                                 const SetTableNameRequest &message)
{
    // TODO: Stub, implement real logic.
    VoidOperationResultResponse response
        {message.queryId_, OperationResult::OK};
    response.Write (Message::VOID_OPERATION_RESULT_RESPONSE, context.session_);
}

void ProcessCreateEditCursorRequest (const ProcessingContext &context,
                                     const TablePartOperationRequest &message)
{
    // TODO: Stub, implement real logic.
    VoidOperationResultResponse response
        {message.queryId_, OperationResult::OK};
    response.Write (Message::VOID_OPERATION_RESULT_RESPONSE, context.session_);
}

void ProcessAddColumnRequest (const ProcessingContext &context,
                              const AddColumnRequest &message)
{
    // TODO: Stub, implement real logic.
    VoidOperationResultResponse response
        {message.queryId_, OperationResult::OK};
    response.Write (Message::VOID_OPERATION_RESULT_RESPONSE, context.session_);
}

void ProcessRemoveColumnRequest (const ProcessingContext &context,
                                 const TablePartOperationRequest &message)
{
    // TODO: Stub, implement real logic.
    VoidOperationResultResponse response
        {message.queryId_, OperationResult::OK};
    response.Write (Message::VOID_OPERATION_RESULT_RESPONSE, context.session_);
}

void ProcessAddIndexRequest (const ProcessingContext &context,
                             const AddIndexRequest &message)
{
    // TODO: Stub, implement real logic.
    VoidOperationResultResponse response
        {message.queryId_, OperationResult::OK};
    response.Write (Message::VOID_OPERATION_RESULT_RESPONSE, context.session_);
}

void ProcessRemoveIndexRequest (const ProcessingContext &context,
                                const TablePartOperationRequest &message)
{
    // TODO: Stub, implement real logic.
    VoidOperationResultResponse response
        {message.queryId_, OperationResult::OK};
    response.Write (Message::VOID_OPERATION_RESULT_RESPONSE, context.session_);
}

void ProcessAddRowRequest (const ProcessingContext &context,
                           const AddRowRequest &message)
{
    // TODO: Stub, implement real logic.
    VoidOperationResultResponse response
        {message.queryId_, OperationResult::OK};
    response.Write (Message::VOID_OPERATION_RESULT_RESPONSE, context.session_);
}

void ProcessCursorAdvanceRequest (const ProcessingContext &context,
                                  const CursorAdvanceRequest &message)
{
    // TODO: Stub, implement real logic.
    VoidOperationResultResponse response
        {message.queryId_, OperationResult::OK};
    response.Write (Message::VOID_OPERATION_RESULT_RESPONSE, context.session_);
}

void ProcessCursorGetRequest (const ProcessingContext &context,
                              const CursorGetRequest &message)
{
    // TODO: Stub, implement real logic.
    VoidOperationResultResponse response
        {message.queryId_, OperationResult::OK};
    response.Write (Message::VOID_OPERATION_RESULT_RESPONSE, context.session_);
}

void ProcessCursorUpdateRequest (const ProcessingContext &context,
                                 const CursorUpdateRequest &message)
{
    // TODO: Stub, implement real logic.
    VoidOperationResultResponse response
        {message.queryId_, OperationResult::OK};
    response.Write (Message::VOID_OPERATION_RESULT_RESPONSE, context.session_);
}

void ProcessCursorDeleteRequest (const ProcessingContext &context,
                                 const CursorVoidActionRequest &message)
{
    // TODO: Stub, implement real logic.
    VoidOperationResultResponse response
        {message.queryId_, OperationResult::OK};
    response.Write (Message::VOID_OPERATION_RESULT_RESPONSE, context.session_);
}

void ProcessCloseCursorRequest (const ProcessingContext &context,
                                const CursorVoidActionRequest &message)
{
    // TODO: Stub, implement real logic.
    VoidOperationResultResponse response
        {message.queryId_, OperationResult::OK};
    response.Write (Message::VOID_OPERATION_RESULT_RESPONSE, context.session_);
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
        },
        [context, request (message)] ()
        {
            SendVoidResult (context, request.queryId_, OperationResult::TIME_OUT);
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
        },
        [context, request (message)] ()
        {
            SendVoidResult (context, request.queryId_, OperationResult::TIME_OUT);
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
        },
        [context, request (message)] ()
        {
            SendVoidResult (context, request.queryId_, OperationResult::TIME_OUT);
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
        },
        [context, request (message)] ()
        {
            SendVoidResult (context, request.queryId_, OperationResult::TIME_OUT);
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
        },
        [context, request (message)] ()
        {
            SendVoidResult (context, request.queryId_, OperationResult::TIME_OUT);
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
        },
        [context, request (message)] ()
        {
            SendVoidResult (context, request.queryId_, OperationResult::TIME_OUT);
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
                    auto tableAccess = GetTableWriteAccessInfo (extension, request.tableId_);
                    if (tableAccess.first == nullptr)
                    {
                        SendVoidResult (context, request.queryId_, OperationResult::TABLE_WRITE_ACCESS_REQUIRED);
                    }
                    else
                    {
                        Richard::ResultCode resultCode = context.databaseConduit_->RemoveTable (
                            extension->conduitWriteGuard_, tableAccess.first, request.tableId_);

                        SendVoidResult (context, request.queryId_,
                                        MapDatabaseResultToOperationResult (resultCode));
                    }
                }
            }
        },
        [context, request (message)] ()
        {
            SendVoidResult (context, request.queryId_, OperationResult::TIME_OUT);
        });
}
}