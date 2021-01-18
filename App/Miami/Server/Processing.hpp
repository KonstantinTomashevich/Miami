#pragma once

#include <memory>
#include <unordered_map>

#include <App/Miami/Messaging/Message.hpp>

#include <Miami/Disco/Disco.hpp>

#include <Miami/Richard/Conduit.hpp>
#include <Miami/Richard/Table.hpp>

#include <Miami/Hotline/SocketSession.hpp>

namespace Miami::App::Server
{
struct ProcessingContext final
{
    Disco::Context *multithreadingContext_;
    Richard::Conduit *databaseConduit_;
    Hotline::SocketSession *session_;
};

struct SessionExtension final
{
    /// Precalculated ISO CRC-64 hash of "Miami::App::Server::SessionExtension".
    static constexpr uint64_t TYPE_ID = 0x3a6ee1b969ccb95f;

    struct TableAccess
    {
        std::shared_ptr <Disco::SafeLockGuard> guard_ = nullptr;
        Richard::Table *tableWeakPointer_ = nullptr;
        bool isWriteAccess_ = false;
    };

    std::shared_ptr <Disco::SafeLockGuard> conduitReadGuard_ = nullptr;
    std::shared_ptr <Disco::SafeLockGuard> conduitWriteGuard_ = nullptr;

    std::unordered_map <Richard::AnyDataId, TableAccess> tableAccesses_ {};
    Richard::AnyDataId nextCursorId_ = 0;
    std::unordered_map <Richard::AnyDataId, std::unique_ptr <Richard::TableReadCursor>> readCursors_ {};
    std::unordered_map <Richard::AnyDataId, std::unique_ptr <Richard::TableEditCursor>> editCursors_ {};
};

void ProcessGetTableReadAccessRequest (const ProcessingContext &context,
                                       const Messaging::TableOperationRequest &message);

void ProcessGetTableWriteAccessRequest (const ProcessingContext &context,
                                        const Messaging::TableOperationRequest &message);

void ProcessCloseTableReadAccessRequest (const ProcessingContext &context,
                                         const Messaging::TableOperationRequest &message);

void ProcessCloseTableWriteAccessRequest (const ProcessingContext &context,
                                          const Messaging::TableOperationRequest &message);

void ProcessGetTableNameRequest (const ProcessingContext &context,
                                 const Messaging::TableOperationRequest &message);

void ProcessCreateReadCursorRequest (const ProcessingContext &context,
                                     const Messaging::TablePartOperationRequest &message);

void ProcessGetColumnsIdsRequest (const ProcessingContext &context,
                                  const Messaging::TableOperationRequest &message);

void ProcessGetColumnInfoRequest (const ProcessingContext &context,
                                  const Messaging::TablePartOperationRequest &message);

void ProcessGetIndicesIdsRequest (const ProcessingContext &context,
                                  const Messaging::TableOperationRequest &message);

void ProcessGetIndexInfoRequest (const ProcessingContext &context,
                                 const Messaging::TablePartOperationRequest &message);

void ProcessSetTableNameRequest (const ProcessingContext &context,
                                 const Messaging::SetTableNameRequest &message);

void ProcessCreateEditCursorRequest (const ProcessingContext &context,
                                     const Messaging::TablePartOperationRequest &message);

void ProcessAddColumnRequest (const ProcessingContext &context,
                              const Messaging::AddColumnRequest &message);

void ProcessRemoveColumnRequest (const ProcessingContext &context,
                                 const Messaging::TablePartOperationRequest &message);

void ProcessAddIndexRequest (const ProcessingContext &context,
                             const Messaging::AddIndexRequest &message);

void ProcessRemoveIndexRequest (const ProcessingContext &context,
                                const Messaging::TablePartOperationRequest &message);

void ProcessAddRowRequest (const ProcessingContext &context,
                           Messaging::AddRowRequest &message);

void ProcessCursorAdvanceRequest (const ProcessingContext &context,
                                  const Messaging::CursorAdvanceRequest &message);

void ProcessCursorGetRequest (const ProcessingContext &context,
                              const Messaging::CursorGetRequest &message);

void ProcessCursorUpdateRequest (const ProcessingContext &context,
                                 const Messaging::CursorUpdateRequest &message);

void ProcessCursorDeleteRequest (const ProcessingContext &context,
                                 const Messaging::CursorVoidActionRequest &message);

void ProcessCloseCursorRequest (const ProcessingContext &context,
                                const Messaging::CursorVoidActionRequest &message);

void ProcessGetConduitReadAccessRequest (const ProcessingContext &context,
                                         const Messaging::ConduitVoidActionRequest &message);

void ProcessGetConduitWriteAccessRequest (const ProcessingContext &context,
                                          const Messaging::ConduitVoidActionRequest &message);

void ProcessCloseConduitReadAccessRequest (const ProcessingContext &context,
                                           const Messaging::ConduitVoidActionRequest &message);

void ProcessCloseConduitWriteAccessRequest (const ProcessingContext &context,
                                            const Messaging::ConduitVoidActionRequest &message);

void ProcessGetTableIdsRequest (const ProcessingContext &context,
                                const Messaging::ConduitVoidActionRequest &message);

void ProcessAddTableRequest (const ProcessingContext &context,
                             const Messaging::AddTableRequest &message);

void ProcessRemoveTableRequest (const ProcessingContext &context,
                                const Messaging::TableOperationRequest &message);
}