// Not only we don't need GDI, but it also has ERROR macro that breaks Evan's LogLevel.
#define NOGDI

#include <App/Miami/Server/Context.hpp>
#include <App/Miami/Server/Processing.hpp>

#include <App/Miami/Messaging/Message.hpp>

#include <Miami/Evan/Logger.hpp>

namespace Miami::App::Server
{
Context::Context (uint32_t workerThreads)
    : alive_ (true),
      multithreadingContext_ (workerThreads),
      databaseConduit_ (&multithreadingContext_),
      socketServer_ (&multithreadingContext_)
{
    Evan::Logger::Get ().Log (
        Evan::LogLevel::INFO,
        "Server context initialized with " + std::to_string (workerThreads) + " worker threads.");
    RegisterMessages ();
}

ResultCode Context::Execute (uint16_t port)
{
    Hotline::ResultCode socketResult = socketServer_.Start (port, false);
    if (socketResult != Hotline::ResultCode::OK)
    {
        Evan::Logger::Get ().Log (
            Evan::LogLevel::ERROR, "Caught Hotline error during socket server deploy " +
                                   std::to_string (static_cast <uint32_t> (socketResult)) + "!");
        return ResultCode::UNABLE_TO_START_SOCKET_SERVER;
    }

    Evan::Logger::Get ().Log (
        Evan::LogLevel::INFO, "Socket server started at port " + std::to_string (port) + ".");

    while (alive_)
    {
        socketResult = socketServer_.CoreContext ().DoStep ();
        if (socketResult != Hotline::ResultCode::OK)
        {
            Evan::Logger::Get ().Log (
                Evan::LogLevel::ERROR, "Caught Hotline error during socket server step " +
                                       std::to_string (static_cast <uint32_t> (socketResult)) + "!");
            alive_ = false;
        }
    }

    Evan::Logger::Get ().Log (
        Evan::LogLevel::INFO, "Shutting down socket server...");
    return ResultCode::OK;
}

void Context::RequestAbort ()
{
    alive_ = false;
}

void Context::RegisterMessages ()
{
    // TODO: For now, only stubs are here.
    // TODO: Handle register failures in release mode.
    Hotline::ResultCode result;

    result = socketServer_.CoreContext ().RegisterFactory (
        static_cast <Hotline::MessageTypeId> (Messaging::Message::GET_TABLE_READ_ACCESS_REQUEST),
        [this] () -> Hotline::MessageParser
        {
            return Messaging::TableOperationRequest::CreateParserWithCallback (
                [this] (const Messaging::TableOperationRequest &message, Hotline::SocketSession *session)
                {
                    Evan::Logger::Get ().Log (
                        Evan::LogLevel::VERBOSE,
                        "Received table " + std::to_string (message.tableId_) +
                        " read access request from session " +
                        std::to_string (reinterpret_cast <ptrdiff_t> (session)) + ".");

                    ProcessGetTableReadAccessRequest (
                        {&multithreadingContext_, &databaseConduit_, session}, message);
                });
        });

    assert (result == Hotline::ResultCode::OK);
    result = socketServer_.CoreContext ().RegisterFactory (
        static_cast <Hotline::MessageTypeId> (Messaging::Message::GET_TABLE_WRITE_ACCESS_REQUEST),
        [this] () -> Hotline::MessageParser
        {
            return Messaging::TableOperationRequest::CreateParserWithCallback (
                [this] (const Messaging::TableOperationRequest &message, Hotline::SocketSession *session)
                {
                    Evan::Logger::Get ().Log (
                        Evan::LogLevel::VERBOSE,
                        "Received table " + std::to_string (message.tableId_) +
                        " write access request from session " +
                        std::to_string (reinterpret_cast <ptrdiff_t> (session)) + ".");

                    ProcessGetTableWriteAccessRequest (
                        {&multithreadingContext_, &databaseConduit_, session}, message);
                });
        });

    assert (result == Hotline::ResultCode::OK);
    result = socketServer_.CoreContext ().RegisterFactory (
        static_cast <Hotline::MessageTypeId> (Messaging::Message::CLOSE_TABLE_READ_ACCESS_REQUEST),
        [this] () -> Hotline::MessageParser
        {
            return Messaging::TableOperationRequest::CreateParserWithCallback (
                [this] (const Messaging::TableOperationRequest &message, Hotline::SocketSession *session)
                {
                    Evan::Logger::Get ().Log (
                        Evan::LogLevel::VERBOSE,
                        "Received table " + std::to_string (message.tableId_) +
                        " read access close request from session " +
                        std::to_string (reinterpret_cast <ptrdiff_t> (session)) + ".");

                    ProcessCloseTableReadAccessRequest (
                        {&multithreadingContext_, &databaseConduit_, session}, message);
                });
        });

    assert (result == Hotline::ResultCode::OK);
    result = socketServer_.CoreContext ().RegisterFactory (
        static_cast <Hotline::MessageTypeId> (Messaging::Message::CLOSE_TABLE_WRITE_ACCESS_REQUEST),
        [this] () -> Hotline::MessageParser
        {
            return Messaging::TableOperationRequest::CreateParserWithCallback (
                [this] (const Messaging::TableOperationRequest &message, Hotline::SocketSession *session)
                {
                    Evan::Logger::Get ().Log (
                        Evan::LogLevel::VERBOSE,
                        "Received table " + std::to_string (message.tableId_) +
                        " write access close request from session " +
                        std::to_string (reinterpret_cast <ptrdiff_t> (session)) + ".");

                    ProcessCloseTableWriteAccessRequest (
                        {&multithreadingContext_, &databaseConduit_, session}, message);
                });
        });

    assert (result == Hotline::ResultCode::OK);
    result = socketServer_.CoreContext ().RegisterFactory (
        static_cast <Hotline::MessageTypeId> (Messaging::Message::GET_TABLE_NAME_REQUEST),
        [this] () -> Hotline::MessageParser
        {
            return Messaging::TableOperationRequest::CreateParserWithCallback (
                [this] (const Messaging::TableOperationRequest &message, Hotline::SocketSession *session)
                {
                    Evan::Logger::Get ().Log (
                        Evan::LogLevel::VERBOSE,
                        "Received table " + std::to_string (message.tableId_) +
                        " name request from session " +
                        std::to_string (reinterpret_cast <ptrdiff_t> (session)) + ".");

                    ProcessGetTableNameRequest (
                        {&multithreadingContext_, &databaseConduit_, session}, message);
                });
        });

    assert (result == Hotline::ResultCode::OK);
    result = socketServer_.CoreContext ().RegisterFactory (
        static_cast <Hotline::MessageTypeId> (Messaging::Message::CREATE_READ_CURSOR_REQUEST),
        [this] () -> Hotline::MessageParser
        {
            return Messaging::TablePartOperationRequest::CreateParserWithCallback (
                [this] (const Messaging::TablePartOperationRequest &message, Hotline::SocketSession *session)
                {
                    Evan::Logger::Get ().Log (
                        Evan::LogLevel::VERBOSE,
                        "Received table " + std::to_string (message.tableId_) +
                        " read cursor creation from index " + std::to_string (message.partId_) +
                        " request from session " + std::to_string (reinterpret_cast <ptrdiff_t> (session)) + ".");

                    ProcessCreateReadCursorRequest (
                        {&multithreadingContext_, &databaseConduit_, session}, message);
                });
        });

    assert (result == Hotline::ResultCode::OK);
    result = socketServer_.CoreContext ().RegisterFactory (
        static_cast <Hotline::MessageTypeId> (Messaging::Message::GET_COLUMNS_IDS_REQUEST),
        [this] () -> Hotline::MessageParser
        {
            return Messaging::TableOperationRequest::CreateParserWithCallback (
                [this] (const Messaging::TableOperationRequest &message, Hotline::SocketSession *session)
                {
                    Evan::Logger::Get ().Log (
                        Evan::LogLevel::VERBOSE,
                        "Received table " + std::to_string (message.tableId_) +
                        " columns ids request from session " +
                        std::to_string (reinterpret_cast <ptrdiff_t> (session)) + ".");

                    ProcessGetColumnsIdsRequest (
                        {&multithreadingContext_, &databaseConduit_, session}, message);
                });
        });

    assert (result == Hotline::ResultCode::OK);
    result = socketServer_.CoreContext ().RegisterFactory (
        static_cast <Hotline::MessageTypeId> (Messaging::Message::GET_COLUMN_INFO_REQUEST),
        [this] () -> Hotline::MessageParser
        {
            return Messaging::TablePartOperationRequest::CreateParserWithCallback (
                [this] (const Messaging::TablePartOperationRequest &message, Hotline::SocketSession *session)
                {
                    Evan::Logger::Get ().Log (
                        Evan::LogLevel::VERBOSE,
                        "Received table " + std::to_string (message.tableId_) +
                        " column " + std::to_string (message.partId_) +
                        " info request from session " + std::to_string (reinterpret_cast <ptrdiff_t> (session)) + ".");

                    ProcessGetColumnInfoRequest (
                        {&multithreadingContext_, &databaseConduit_, session}, message);
                });
        });

    assert (result == Hotline::ResultCode::OK);
    result = socketServer_.CoreContext ().RegisterFactory (
        static_cast <Hotline::MessageTypeId> (Messaging::Message::GET_INDICES_IDS_REQUEST),
        [this] () -> Hotline::MessageParser
        {
            return Messaging::TableOperationRequest::CreateParserWithCallback (
                [this] (const Messaging::TableOperationRequest &message, Hotline::SocketSession *session)
                {
                    Evan::Logger::Get ().Log (
                        Evan::LogLevel::VERBOSE,
                        "Received table " + std::to_string (message.tableId_) +
                        " indices ids request from session " +
                        std::to_string (reinterpret_cast <ptrdiff_t> (session)) + ".");

                    ProcessGetIndicesIdsRequest (
                        {&multithreadingContext_, &databaseConduit_, session}, message);
                });
        });

    assert (result == Hotline::ResultCode::OK);
    result = socketServer_.CoreContext ().RegisterFactory (
        static_cast <Hotline::MessageTypeId> (Messaging::Message::GET_INDEX_INFO_REQUEST),
        [this] () -> Hotline::MessageParser
        {
            return Messaging::TablePartOperationRequest::CreateParserWithCallback (
                [this] (const Messaging::TablePartOperationRequest &message, Hotline::SocketSession *session)
                {
                    Evan::Logger::Get ().Log (
                        Evan::LogLevel::VERBOSE,
                        "Received table " + std::to_string (message.tableId_) +
                        " index " + std::to_string (message.partId_) +
                        " info request from session " + std::to_string (reinterpret_cast <ptrdiff_t> (session)) + ".");

                    ProcessGetIndexInfoRequest (
                        {&multithreadingContext_, &databaseConduit_, session}, message);
                });
        });

    assert (result == Hotline::ResultCode::OK);
    result = socketServer_.CoreContext ().RegisterFactory (
        static_cast <Hotline::MessageTypeId> (Messaging::Message::SET_TABLE_NAME_REQUEST),
        [this] () -> Hotline::MessageParser
        {
            return Messaging::SetTableNameRequest::CreateParserWithCallback (
                [this] (const Messaging::SetTableNameRequest &message, Hotline::SocketSession *session)
                {
                    Evan::Logger::Get ().Log (
                        Evan::LogLevel::VERBOSE,
                        "Received table " + std::to_string (message.tableId_) +
                        " name change to \"" + message.newName_ +
                        "\" request from session " + std::to_string (reinterpret_cast <ptrdiff_t> (session)) + ".");

                    ProcessSetTableNameRequest (
                        {&multithreadingContext_, &databaseConduit_, session}, message);
                });
        });

    assert (result == Hotline::ResultCode::OK);
    result = socketServer_.CoreContext ().RegisterFactory (
        static_cast <Hotline::MessageTypeId> (Messaging::Message::CREATE_EDIT_CURSOR_REQUEST),
        [this] () -> Hotline::MessageParser
        {
            return Messaging::TablePartOperationRequest::CreateParserWithCallback (
                [this] (const Messaging::TablePartOperationRequest &message, Hotline::SocketSession *session)
                {
                    Evan::Logger::Get ().Log (
                        Evan::LogLevel::VERBOSE,
                        "Received table " + std::to_string (message.tableId_) +
                        " edit cursor creation from index " + std::to_string (message.partId_) +
                        " request from session " + std::to_string (reinterpret_cast <ptrdiff_t> (session)) + ".");

                    ProcessCreateEditCursorRequest (
                        {&multithreadingContext_, &databaseConduit_, session}, message);
                });
        });

    assert (result == Hotline::ResultCode::OK);
    result = socketServer_.CoreContext ().RegisterFactory (
        static_cast <Hotline::MessageTypeId> (Messaging::Message::ADD_COLUMN_REQUEST),
        [this] () -> Hotline::MessageParser
        {
            return Messaging::AddColumnRequest::CreateParserWithCallback (
                [this] (const Messaging::AddColumnRequest &message, Hotline::SocketSession *session)
                {
                    Evan::Logger::Get ().Log (
                        Evan::LogLevel::VERBOSE,
                        "Received table " + std::to_string (message.tableId_) +
                        " add column \"" + message.name_ + "\": " +
                        Richard::GetDataTypeName (message.dataType_) +
                        " request from session " + std::to_string (reinterpret_cast <ptrdiff_t> (session)) + ".");

                    ProcessAddColumnRequest (
                        {&multithreadingContext_, &databaseConduit_, session}, message);
                });
        });

    assert (result == Hotline::ResultCode::OK);
    result = socketServer_.CoreContext ().RegisterFactory (
        static_cast <Hotline::MessageTypeId> (Messaging::Message::REMOVE_COLUMN_REQUEST),
        [this] () -> Hotline::MessageParser
        {
            return Messaging::TablePartOperationRequest::CreateParserWithCallback (
                [this] (const Messaging::TablePartOperationRequest &message, Hotline::SocketSession *session)
                {
                    Evan::Logger::Get ().Log (
                        Evan::LogLevel::VERBOSE,
                        "Received table " + std::to_string (message.tableId_) +
                        " column " + std::to_string (message.partId_) +
                        " remove request from session " + std::to_string (reinterpret_cast <ptrdiff_t> (session)) +
                        ".");

                    ProcessRemoveColumnRequest (
                        {&multithreadingContext_, &databaseConduit_, session}, message);
                });
        });

    assert (result == Hotline::ResultCode::OK);
    result = socketServer_.CoreContext ().RegisterFactory (
        static_cast <Hotline::MessageTypeId> (Messaging::Message::ADD_INDEX_REQUEST),
        [this] () -> Hotline::MessageParser
        {
            return Messaging::AddIndexRequest::CreateParserWithCallback (
                [this] (const Messaging::AddIndexRequest &message, Hotline::SocketSession *session)
                {
                    // TODO: Print columns list?
                    Evan::Logger::Get ().Log (
                        Evan::LogLevel::VERBOSE,
                        "Received table " + std::to_string (message.tableId_) +
                        " add index \"" + message.name_ +
                        "\" request from session " + std::to_string (reinterpret_cast <ptrdiff_t> (session)) + ".");

                    ProcessAddIndexRequest (
                        {&multithreadingContext_, &databaseConduit_, session}, message);
                });
        });

    assert (result == Hotline::ResultCode::OK);
    result = socketServer_.CoreContext ().RegisterFactory (
        static_cast <Hotline::MessageTypeId> (Messaging::Message::REMOVE_INDEX_REQUEST),
        [this] () -> Hotline::MessageParser
        {
            return Messaging::TablePartOperationRequest::CreateParserWithCallback (
                [this] (const Messaging::TablePartOperationRequest &message, Hotline::SocketSession *session)
                {
                    Evan::Logger::Get ().Log (
                        Evan::LogLevel::VERBOSE,
                        "Received table " + std::to_string (message.tableId_) +
                        " index " + std::to_string (message.partId_) +
                        " remove request from session " + std::to_string (reinterpret_cast <ptrdiff_t> (session)) +
                        ".");

                    ProcessRemoveIndexRequest (
                        {&multithreadingContext_, &databaseConduit_, session}, message);
                });
        });

    assert (result == Hotline::ResultCode::OK);
    result = socketServer_.CoreContext ().RegisterFactory (
        static_cast <Hotline::MessageTypeId> (Messaging::Message::ADD_ROW_REQUEST),
        [this] () -> Hotline::MessageParser
        {
            return Messaging::AddRowRequest::CreateParserWithCallback (
                [this] (const Messaging::AddRowRequest &message, Hotline::SocketSession *session)
                {
                    // TODO: Print more info?
                    Evan::Logger::Get ().Log (
                        Evan::LogLevel::VERBOSE,
                        "Received table " + std::to_string (message.tableId_) +
                        " add row request from session " +
                        std::to_string (reinterpret_cast <ptrdiff_t> (session)) + ".");

                    ProcessAddRowRequest (
                        {&multithreadingContext_, &databaseConduit_, session}, message);
                });
        });

    assert (result == Hotline::ResultCode::OK);
    result = socketServer_.CoreContext ().RegisterFactory (
        static_cast <Hotline::MessageTypeId> (Messaging::Message::CURSOR_ADVANCE_REQUEST),
        [this] () -> Hotline::MessageParser
        {
            return Messaging::CursorAdvanceRequest::CreateParserWithCallback (
                [this] (const Messaging::CursorAdvanceRequest &message, Hotline::SocketSession *session)
                {
                    Evan::Logger::Get ().Log (
                        Evan::LogLevel::VERBOSE,
                        "Received cursor " + std::to_string (message.cursorId_) +
                        " advance by " + std::to_string (message.step_) + " request from session " +
                        std::to_string (reinterpret_cast <ptrdiff_t> (session)) + ".");

                    ProcessCursorAdvanceRequest (
                        {&multithreadingContext_, &databaseConduit_, session}, message);
                });
        });

    assert (result == Hotline::ResultCode::OK);
    result = socketServer_.CoreContext ().RegisterFactory (
        static_cast <Hotline::MessageTypeId> (Messaging::Message::CURSOR_GET_REQUEST),
        [this] () -> Hotline::MessageParser
        {
            return Messaging::CursorGetRequest::CreateParserWithCallback (
                [this] (const Messaging::CursorGetRequest &message, Hotline::SocketSession *session)
                {
                    Evan::Logger::Get ().Log (
                        Evan::LogLevel::VERBOSE,
                        "Received cursor " + std::to_string (message.cursorId_) +
                        " get column " + std::to_string (message.columnId_) + " request from session " +
                        std::to_string (reinterpret_cast <ptrdiff_t> (session)) + ".");

                    ProcessCursorGetRequest (
                        {&multithreadingContext_, &databaseConduit_, session}, message);
                });
        });

    assert (result == Hotline::ResultCode::OK);
    result = socketServer_.CoreContext ().RegisterFactory (
        static_cast <Hotline::MessageTypeId> (Messaging::Message::CURSOR_UPDATE_REQUEST),
        [this] () -> Hotline::MessageParser
        {
            return Messaging::CursorUpdateRequest::CreateParserWithCallback (
                [this] (const Messaging::CursorUpdateRequest &message, Hotline::SocketSession *session)
                {
                    // TODO: Print more info?
                    Evan::Logger::Get ().Log (
                        Evan::LogLevel::VERBOSE,
                        "Received cursor " + std::to_string (message.cursorId_) +
                        " update request from session " +
                        std::to_string (reinterpret_cast <ptrdiff_t> (session)) + ".");

                    ProcessCursorUpdateRequest (
                        {&multithreadingContext_, &databaseConduit_, session}, message);
                });
        });

    assert (result == Hotline::ResultCode::OK);
    result = socketServer_.CoreContext ().RegisterFactory (
        static_cast <Hotline::MessageTypeId> (Messaging::Message::CURSOR_DELETE_REQUEST),
        [this] () -> Hotline::MessageParser
        {
            return Messaging::CursorVoidActionRequest::CreateParserWithCallback (
                [this] (const Messaging::CursorVoidActionRequest &message, Hotline::SocketSession *session)
                {
                    Evan::Logger::Get ().Log (
                        Evan::LogLevel::VERBOSE,
                        "Received cursor " + std::to_string (message.cursorId_) +
                        " delete current request from session " +
                        std::to_string (reinterpret_cast <ptrdiff_t> (session)) + ".");

                    ProcessCursorDeleteRequest (
                        {&multithreadingContext_, &databaseConduit_, session}, message);
                });
        });

    assert (result == Hotline::ResultCode::OK);
    result = socketServer_.CoreContext ().RegisterFactory (
        static_cast <Hotline::MessageTypeId> (Messaging::Message::CLOSE_CURSOR_REQUEST),
        [this] () -> Hotline::MessageParser
        {
            return Messaging::CursorVoidActionRequest::CreateParserWithCallback (
                [this] (const Messaging::CursorVoidActionRequest &message, Hotline::SocketSession *session)
                {
                    Evan::Logger::Get ().Log (
                        Evan::LogLevel::VERBOSE,
                        "Received cursor " + std::to_string (message.cursorId_) +
                        " close request from session " +
                        std::to_string (reinterpret_cast <ptrdiff_t> (session)) + ".");

                    ProcessCloseCursorRequest (
                        {&multithreadingContext_, &databaseConduit_, session}, message);
                });
        });

    assert (result == Hotline::ResultCode::OK);
    result = socketServer_.CoreContext ().RegisterFactory (
        static_cast <Hotline::MessageTypeId> (Messaging::Message::GET_CONDUIT_READ_ACCESS_REQUEST),
        [this] () -> Hotline::MessageParser
        {
            return Messaging::ConduitVoidActionRequest::CreateParserWithCallback (
                [this] (const Messaging::ConduitVoidActionRequest &message, Hotline::SocketSession *session)
                {
                    Evan::Logger::Get ().Log (
                        Evan::LogLevel::VERBOSE,
                        "Received conduit read access request from session " +
                        std::to_string (reinterpret_cast <ptrdiff_t> (session)) + ".");

                    ProcessGetConduitReadAccessRequest (
                        {&multithreadingContext_, &databaseConduit_, session}, message);
                });
        });

    assert (result == Hotline::ResultCode::OK);
    result = socketServer_.CoreContext ().RegisterFactory (
        static_cast <Hotline::MessageTypeId> (Messaging::Message::GET_CONDUIT_WRITE_ACCESS_REQUEST),
        [this] () -> Hotline::MessageParser
        {
            return Messaging::ConduitVoidActionRequest::CreateParserWithCallback (
                [this] (const Messaging::ConduitVoidActionRequest &message, Hotline::SocketSession *session)
                {
                    Evan::Logger::Get ().Log (
                        Evan::LogLevel::VERBOSE,
                        "Received conduit write access request from session " +
                        std::to_string (reinterpret_cast <ptrdiff_t> (session)) + ".");

                    ProcessGetConduitWriteAccessRequest (
                        {&multithreadingContext_, &databaseConduit_, session}, message);
                });
        });

    assert (result == Hotline::ResultCode::OK);
    result = socketServer_.CoreContext ().RegisterFactory (
        static_cast <Hotline::MessageTypeId> (Messaging::Message::CLOSE_CONDUIT_READ_ACCESS_REQUEST),
        [this] () -> Hotline::MessageParser
        {
            return Messaging::ConduitVoidActionRequest::CreateParserWithCallback (
                [this] (const Messaging::ConduitVoidActionRequest &message, Hotline::SocketSession *session)
                {
                    Evan::Logger::Get ().Log (
                        Evan::LogLevel::VERBOSE,
                        "Received conduit read access close request from session " +
                        std::to_string (reinterpret_cast <ptrdiff_t> (session)) + ".");

                    ProcessCloseConduitReadAccessRequest (
                        {&multithreadingContext_, &databaseConduit_, session}, message);
                });
        });

    assert (result == Hotline::ResultCode::OK);
    result = socketServer_.CoreContext ().RegisterFactory (
        static_cast <Hotline::MessageTypeId> (Messaging::Message::CLOSE_CONDUIT_WRITE_ACCESS_REQUEST),
        [this] () -> Hotline::MessageParser
        {
            return Messaging::ConduitVoidActionRequest::CreateParserWithCallback (
                [this] (const Messaging::ConduitVoidActionRequest &message, Hotline::SocketSession *session)
                {
                    Evan::Logger::Get ().Log (
                        Evan::LogLevel::VERBOSE,
                        "Received conduit write access close request from session " +
                        std::to_string (reinterpret_cast <ptrdiff_t> (session)) + ".");

                    ProcessCloseConduitWriteAccessRequest (
                        {&multithreadingContext_, &databaseConduit_, session}, message);
                });
        });

    assert (result == Hotline::ResultCode::OK);
    result = socketServer_.CoreContext ().RegisterFactory (
        static_cast <Hotline::MessageTypeId> (Messaging::Message::GET_TABLE_IDS_REQUEST),
        [this] () -> Hotline::MessageParser
        {
            return Messaging::ConduitVoidActionRequest::CreateParserWithCallback (
                [this] (const Messaging::ConduitVoidActionRequest &message, Hotline::SocketSession *session)
                {
                    Evan::Logger::Get ().Log (
                        Evan::LogLevel::VERBOSE,
                        "Received conduit table ids request from session " +
                        std::to_string (reinterpret_cast <ptrdiff_t> (session)) + ".");

                    ProcessGetTableIdsRequest (
                        {&multithreadingContext_, &databaseConduit_, session}, message);
                });
        });

    assert (result == Hotline::ResultCode::OK);
    result = socketServer_.CoreContext ().RegisterFactory (
        static_cast <Hotline::MessageTypeId> (Messaging::Message::ADD_TABLE_REQUEST),
        [this] () -> Hotline::MessageParser
        {
            return Messaging::AddTableRequest::CreateParserWithCallback (
                [this] (const Messaging::AddTableRequest &message, Hotline::SocketSession *session)
                {
                    Evan::Logger::Get ().Log (
                        Evan::LogLevel::VERBOSE,
                        "Received conduit add table \"" + message.tableName_ +
                        "\" request from session " + std::to_string (reinterpret_cast <ptrdiff_t> (session)) + ".");

                    ProcessAddTableRequest (
                        {&multithreadingContext_, &databaseConduit_, session}, message);
                });
        });

    assert (result == Hotline::ResultCode::OK);
    result = socketServer_.CoreContext ().RegisterFactory (
        static_cast <Hotline::MessageTypeId> (Messaging::Message::REMOVE_TABLE_REQUEST),
        [this] () -> Hotline::MessageParser
        {
            return Messaging::TableOperationRequest::CreateParserWithCallback (
                [this] (const Messaging::TableOperationRequest &message, Hotline::SocketSession *session)
                {
                    Evan::Logger::Get ().Log (
                        Evan::LogLevel::VERBOSE,
                        "Received table " + std::to_string (message.tableId_) +
                        " remove request from session " +
                        std::to_string (reinterpret_cast <ptrdiff_t> (session)) + ".");

                    ProcessRemoveTableRequest (
                        {&multithreadingContext_, &databaseConduit_, session}, message);
                });
        });

    assert (result == Hotline::ResultCode::OK);
}
}
