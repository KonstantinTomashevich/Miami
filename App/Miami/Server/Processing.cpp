#include <App/Miami/Server/Processing.hpp>

namespace Miami::App::Server
{
void ProcessGetTableReadAccessRequest (const ProcessingContext &context,
                                       const Messaging::TableOperationRequest &message)
{
    // TODO: Stub, implement real logic.
    Messaging::VoidOperationResultResponse response
        {message.queryId_, Messaging::OperationResult::OK};
    response.Write (Messaging::Message::VOID_OPERATION_RESULT_RESPONSE, context.session_);
}

void ProcessGetTableWriteAccessRequest (const ProcessingContext &context,
                                        const Messaging::TableOperationRequest &message)
{
    // TODO: Stub, implement real logic.
    Messaging::VoidOperationResultResponse response
        {message.queryId_, Messaging::OperationResult::OK};
    response.Write (Messaging::Message::VOID_OPERATION_RESULT_RESPONSE, context.session_);
}

void ProcessCloseTableReadAccessRequest (const ProcessingContext &context,
                                    const Messaging::TableOperationRequest &message)
{
    // TODO: Stub, implement real logic.
    Messaging::VoidOperationResultResponse response
        {message.queryId_, Messaging::OperationResult::OK};
    response.Write (Messaging::Message::VOID_OPERATION_RESULT_RESPONSE, context.session_);
}

void ProcessCloseTableWriteAccessRequest (const ProcessingContext &context,
                                     const Messaging::TableOperationRequest &message)
{
    // TODO: Stub, implement real logic.
    Messaging::VoidOperationResultResponse response
        {message.queryId_, Messaging::OperationResult::OK};
    response.Write (Messaging::Message::VOID_OPERATION_RESULT_RESPONSE, context.session_);
}

void ProcessGetTableNameRequest (const ProcessingContext &context,
                                 const Messaging::TableOperationRequest &message)
{
    // TODO: Stub, implement real logic.
    Messaging::VoidOperationResultResponse response
        {message.queryId_, Messaging::OperationResult::OK};
    response.Write (Messaging::Message::VOID_OPERATION_RESULT_RESPONSE, context.session_);
}

void ProcessCreateReadCursorRequest (const ProcessingContext &context,
                                const Messaging::TablePartOperationRequest &message)
{
    // TODO: Stub, implement real logic.
    Messaging::VoidOperationResultResponse response
        {message.queryId_, Messaging::OperationResult::OK};
    response.Write (Messaging::Message::VOID_OPERATION_RESULT_RESPONSE, context.session_);
}

void ProcessGetColumnsIdsRequest (const ProcessingContext &context,
                                  const Messaging::TableOperationRequest &message)
{
    // TODO: Stub, implement real logic.
    Messaging::VoidOperationResultResponse response
        {message.queryId_, Messaging::OperationResult::OK};
    response.Write (Messaging::Message::VOID_OPERATION_RESULT_RESPONSE, context.session_);
}

void ProcessGetColumnInfoRequest (const ProcessingContext &context,
                                  const Messaging::TablePartOperationRequest &message)
{
    // TODO: Stub, implement real logic.
    Messaging::VoidOperationResultResponse response
        {message.queryId_, Messaging::OperationResult::OK};
    response.Write (Messaging::Message::VOID_OPERATION_RESULT_RESPONSE, context.session_);
}

void ProcessGetIndicesIdsRequest (const ProcessingContext &context,
                                  const Messaging::TableOperationRequest &message)
{
    // TODO: Stub, implement real logic.
    Messaging::VoidOperationResultResponse response
        {message.queryId_, Messaging::OperationResult::OK};
    response.Write (Messaging::Message::VOID_OPERATION_RESULT_RESPONSE, context.session_);
}

void ProcessGetIndexInfoRequest (const ProcessingContext &context,
                                 const Messaging::TablePartOperationRequest &message)
{
    // TODO: Stub, implement real logic.
    Messaging::VoidOperationResultResponse response
        {message.queryId_, Messaging::OperationResult::OK};
    response.Write (Messaging::Message::VOID_OPERATION_RESULT_RESPONSE, context.session_);
}

void ProcessSetTableNameRequest (const ProcessingContext &context,
                                 const Messaging::SetTableNameRequest &message)
{
    // TODO: Stub, implement real logic.
    Messaging::VoidOperationResultResponse response
        {message.queryId_, Messaging::OperationResult::OK};
    response.Write (Messaging::Message::VOID_OPERATION_RESULT_RESPONSE, context.session_);
}

void ProcessCreateEditCursorRequest (const ProcessingContext &context,
                                const Messaging::TablePartOperationRequest &message)
{
    // TODO: Stub, implement real logic.
    Messaging::VoidOperationResultResponse response
        {message.queryId_, Messaging::OperationResult::OK};
    response.Write (Messaging::Message::VOID_OPERATION_RESULT_RESPONSE, context.session_);
}

void ProcessAddColumnRequest (const ProcessingContext &context,
                              const Messaging::AddColumnRequest &message)
{
    // TODO: Stub, implement real logic.
    Messaging::VoidOperationResultResponse response
        {message.queryId_, Messaging::OperationResult::OK};
    response.Write (Messaging::Message::VOID_OPERATION_RESULT_RESPONSE, context.session_);
}

void ProcessRemoveColumnRequest (const ProcessingContext &context,
                                 const Messaging::TablePartOperationRequest &message)
{
    // TODO: Stub, implement real logic.
    Messaging::VoidOperationResultResponse response
        {message.queryId_, Messaging::OperationResult::OK};
    response.Write (Messaging::Message::VOID_OPERATION_RESULT_RESPONSE, context.session_);
}

void ProcessAddIndexRequest (const ProcessingContext &context,
                             const Messaging::AddIndexRequest &message)
{
    // TODO: Stub, implement real logic.
    Messaging::VoidOperationResultResponse response
        {message.queryId_, Messaging::OperationResult::OK};
    response.Write (Messaging::Message::VOID_OPERATION_RESULT_RESPONSE, context.session_);
}

void ProcessRemoveIndexRequest (const ProcessingContext &context,
                                const Messaging::TablePartOperationRequest &message)
{
    // TODO: Stub, implement real logic.
    Messaging::VoidOperationResultResponse response
        {message.queryId_, Messaging::OperationResult::OK};
    response.Write (Messaging::Message::VOID_OPERATION_RESULT_RESPONSE, context.session_);
}

void ProcessAddRowRequest (const ProcessingContext &context,
                           const Messaging::AddRowRequest &message)
{
    // TODO: Stub, implement real logic.
    Messaging::VoidOperationResultResponse response
        {message.queryId_, Messaging::OperationResult::OK};
    response.Write (Messaging::Message::VOID_OPERATION_RESULT_RESPONSE, context.session_);
}

void ProcessCursorAdvanceRequest (const ProcessingContext &context,
                                  const Messaging::CursorAdvanceRequest &message)
{
    // TODO: Stub, implement real logic.
    Messaging::VoidOperationResultResponse response
        {message.queryId_, Messaging::OperationResult::OK};
    response.Write (Messaging::Message::VOID_OPERATION_RESULT_RESPONSE, context.session_);
}

void ProcessCursorGetRequest (const ProcessingContext &context,
                              const Messaging::CursorGetRequest &message)
{
    // TODO: Stub, implement real logic.
    Messaging::VoidOperationResultResponse response
        {message.queryId_, Messaging::OperationResult::OK};
    response.Write (Messaging::Message::VOID_OPERATION_RESULT_RESPONSE, context.session_);
}

void ProcessCursorUpdateRequest (const ProcessingContext &context,
                                 const Messaging::CursorUpdateRequest &message)
{
    // TODO: Stub, implement real logic.
    Messaging::VoidOperationResultResponse response
        {message.queryId_, Messaging::OperationResult::OK};
    response.Write (Messaging::Message::VOID_OPERATION_RESULT_RESPONSE, context.session_);
}

void ProcessCursorDeleteRequest (const ProcessingContext &context,
                                 const Messaging::CursorVoidActionRequest &message)
{
    // TODO: Stub, implement real logic.
    Messaging::VoidOperationResultResponse response
        {message.queryId_, Messaging::OperationResult::OK};
    response.Write (Messaging::Message::VOID_OPERATION_RESULT_RESPONSE, context.session_);
}

void ProcessCloseCursorRequest (const ProcessingContext &context,
                                const Messaging::CursorVoidActionRequest &message)
{
    // TODO: Stub, implement real logic.
    Messaging::VoidOperationResultResponse response
        {message.queryId_, Messaging::OperationResult::OK};
    response.Write (Messaging::Message::VOID_OPERATION_RESULT_RESPONSE, context.session_);
}

void ProcessGetConduitReadAccessRequest (const ProcessingContext &context,
                                         const Messaging::ConduitVoidActionRequest &message)
{
    // TODO: Stub, implement real logic.
    Messaging::VoidOperationResultResponse response
        {message.queryId_, Messaging::OperationResult::OK};
    response.Write (Messaging::Message::VOID_OPERATION_RESULT_RESPONSE, context.session_);
}

void ProcessGetConduitWriteAccessRequest (const ProcessingContext &context,
                                          const Messaging::ConduitVoidActionRequest &message)
{
    // TODO: Stub, implement real logic.
    Messaging::VoidOperationResultResponse response
        {message.queryId_, Messaging::OperationResult::OK};
    response.Write (Messaging::Message::VOID_OPERATION_RESULT_RESPONSE, context.session_);
}

void ProcessCloseConduitReadAccessRequest (const ProcessingContext &context,
                                           const Messaging::ConduitVoidActionRequest &message)
{
    // TODO: Stub, implement real logic.
    Messaging::VoidOperationResultResponse response
        {message.queryId_, Messaging::OperationResult::OK};
    response.Write (Messaging::Message::VOID_OPERATION_RESULT_RESPONSE, context.session_);
}

void ProcessCloseConduitWriteAccessRequest (const ProcessingContext &context,
                                            const Messaging::ConduitVoidActionRequest &message)
{
    // TODO: Stub, implement real logic.
    Messaging::VoidOperationResultResponse response
        {message.queryId_, Messaging::OperationResult::OK};
    response.Write (Messaging::Message::VOID_OPERATION_RESULT_RESPONSE, context.session_);
}

void ProcessGetTableIdsRequest (const ProcessingContext &context,
                                const Messaging::ConduitVoidActionRequest &message)
{
    // TODO: Stub, implement real logic.
    Messaging::VoidOperationResultResponse response
        {message.queryId_, Messaging::OperationResult::OK};
    response.Write (Messaging::Message::VOID_OPERATION_RESULT_RESPONSE, context.session_);
}

void ProcessAddTableRequest (const ProcessingContext &context,
                             const Messaging::AddTableRequest &message)
{
    // TODO: Stub, implement real logic.
    Messaging::VoidOperationResultResponse response
        {message.queryId_, Messaging::OperationResult::OK};
    response.Write (Messaging::Message::VOID_OPERATION_RESULT_RESPONSE, context.session_);
}

void ProcessRemoveTableRequest (const ProcessingContext &context,
                                const Messaging::TableOperationRequest &message)
{
    // TODO: Stub, implement real logic.
    Messaging::VoidOperationResultResponse response
        {message.queryId_, Messaging::OperationResult::OK};
    response.Write (Messaging::Message::VOID_OPERATION_RESULT_RESPONSE, context.session_);
}
}