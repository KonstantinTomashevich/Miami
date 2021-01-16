#pragma once

#include <cassert>

#include <Miami/Hotline/Message.hpp>

#include <Miami/Richard/Data.hpp>

#include <Miami/Hotline/SocketSession.hpp>

namespace Miami::App::Messaging
{
using QueryId = uint64_t;
using ResourceId = Richard::AnyDataId;

enum class Message
{
    VOID_OPERATION_RESULT_RESPONSE,
    CREATE_OPERATION_RESULT_RESPONSE,
    RESOURCE_IDS_RESPONSE,

    GET_TABLE_READ_ACCESS_REQUEST, // -> VOID_OPERATION_RESULT_RESPONSE
    GET_TABLE_WRITE_ACCESS_REQUEST, // -> VOID_OPERATION_RESULT_RESPONSE

    CLOSE_TABLE_READ_ACCESS_REQUEST, // -> VOID_OPERATION_RESULT_RESPONSE
    CLOSE_TABLE_WRITE_ACCESS_REQUEST, // -> VOID_OPERATION_RESULT_RESPONSE

    GET_TABLE_NAME_REQUEST, // -> GET_TABLE_NAME_RESPONSE ||
    //                            VOID_OPERATION_RESULT_RESPONSE
    GET_TABLE_NAME_RESPONSE,

    CREATE_READ_CURSOR_REQUEST, // -> CREATE_OPERATION_RESULT_RESPONSE ||
    //                                VOID_OPERATION_RESULT_RESPONSE

    GET_COLUMNS_IDS_REQUEST, // -> RESOURCE_IDS_RESPONSE ||
    //                             VOID_OPERATION_RESULT_RESPONSE

    GET_COLUMN_INFO_REQUEST, // -> GET_COLUMN_INFO_RESPONSE ||
    //                             VOID_OPERATION_RESULT_RESPONSE
    GET_COLUMN_INFO_RESPONSE,

    GET_INDICES_IDS_REQUEST, // -> RESOURCE_IDS_RESPONSE ||
    //                             VOID_OPERATION_RESULT_RESPONSE

    GET_INDEX_INFO_REQUEST, // -> GET_INDEX_INFO_RESPONSE ||
    //                            VOID_OPERATION_RESULT_RESPONSE
    GET_INDEX_INFO_RESPONSE,

    SET_TABLE_NAME_REQUEST, // -> VOID_OPERATION_RESULT_RESPONSE
    CREATE_EDIT_CURSOR_REQUEST, // -> CREATE_OPERATION_RESULT_RESPONSE ||
    //                                VOID_OPERATION_RESULT_RESPONSE

    ADD_COLUMN_REQUEST, // -> CREATE_OPERATION_RESULT_RESPONSE ||
    //                        VOID_OPERATION_RESULT_RESPONSE

    REMOVE_COLUMN_REQUEST, // -> VOID_OPERATION_RESULT_RESPONSE
    ADD_INDEX_REQUEST, // -> CREATE_OPERATION_RESULT_RESPONSE ||
    //                       VOID_OPERATION_RESULT_RESPONSE

    REMOVE_INDEX_REQUEST, // -> VOID_OPERATION_RESULT_RESPONSE
    ADD_ROW_REQUEST, // -> CREATE_OPERATION_RESULT_RESPONSE ||
    //                     VOID_OPERATION_RESULT_RESPONSE

    CURSOR_ADVANCE_REQUEST, // -> VOID_OPERATION_RESULT_RESPONSE

    CURSOR_GET_REQUEST, // -> CURSOR_GET_RESPONSE ||
    //                        VOID_OPERATION_RESULT_RESPONSE
    CURSOR_GET_RESPONSE,

    CURSOR_UPDATE_REQUEST, // -> VOID_OPERATION_RESULT_RESPONSE
    CURSOR_DELETE_REQUEST, // -> VOID_OPERATION_RESULT_RESPONSE
    CLOSE_CURSOR_REQUEST, // -> VOID_OPERATION_RESULT_RESPONSE

    GET_CONDUIT_READ_ACCESS_REQUEST, // -> VOID_OPERATION_RESULT_RESPONSE
    GET_CONDUIT_WRITE_ACCESS_REQUEST, // -> VOID_OPERATION_RESULT_RESPONSE

    CLOSE_CONDUIT_READ_ACCESS_REQUEST, // -> VOID_OPERATION_RESULT_RESPONSE
    CLOSE_CONDUIT_WRITE_ACCESS_REQUEST, // -> VOID_OPERATION_RESULT_RESPONSE

    GET_TABLE_IDS_REQUEST, // -> RESOURCE_IDS_RESPONSE ||
    //                           VOID_OPERATION_RESULT_RESPONSE

    ADD_TABLE_REQUEST, // -> CREATE_OPERATION_RESULT_RESPONSE ||
    //                       VOID_OPERATION_RESULT_RESPONSE
    REMOVE_TABLE_REQUEST, // -> VOID_OPERATION_RESULT_RESPONSE
};

enum class OperationResult
{
    // TODO: Fill during server development.
};

/// For message VOID_OPERATION_RESULT_RESPONSE.
struct VoidOperationResultResponse
{
    static Hotline::MessageParser CreateParserWithCallback (
        std::function <void (VoidOperationResultResponse &, Hotline::SocketSession *)> &&callback);

    QueryId queryId_;
    OperationResult result_;

    void Write (Hotline::SocketSession *session) const;
};

static_assert (std::is_pod_v <VoidOperationResultResponse>);

/// For messages:
/// - GET_TABLE_READ_ACCESS_REQUEST.
/// - GET_TABLE_WRITE_ACCESS_REQUEST.
/// - CLOSE_TABLE_READ_ACCESS_REQUEST.
/// - CLOSE_TABLE_WRITE_ACCESS_REQUEST.
/// - GET_TABLE_NAME_REQUEST.
/// - GET_COLUMNS_IDS_REQUEST.
/// - GET_INDICES_IDS_REQUEST.
/// - REMOVE_TABLE_REQUEST.
struct TableOperationRequest
{
    static Hotline::MessageParser CreateParserWithCallback (
        std::function <void (TableOperationRequest &, Hotline::SocketSession *)> &&callback);

    QueryId queryId_;
    ResourceId tableId_;

    void Write (Hotline::SocketSession *session) const;
};

static_assert (std::is_pod_v <TableOperationRequest>);

/// For message GET_TABLE_NAME_RESPONSE.
struct GetTableNameResponse
{
    static Hotline::MessageParser CreateParserWithCallback (
        std::function <void (GetTableNameResponse &, Hotline::SocketSession *)> &&callback);

    QueryId queryId_;
    std::string tableName_;

    void Write (Hotline::SocketSession *session) const;
};

/// For messages:
/// - CREATE_READ_CURSOR_REQUEST.
/// - GET_COLUMN_INFO_REQUEST.
/// - GET_INDEX_INFO_REQUEST.
/// - CREATE_EDIT_CURSOR_REQUEST.
/// - REMOVE_COLUMN_REQUEST.
/// - REMOVE_INDEX_REQUEST.
struct TablePartOperationRequest
{
    static Hotline::MessageParser CreateParserWithCallback (
        std::function <void (TablePartOperationRequest &, Hotline::SocketSession *)> &&callback);

    QueryId queryId_;
    ResourceId tableId_;
    ResourceId partId_;

    void Write (Hotline::SocketSession *session) const;
};


/// For message CREATE_OPERATION_RESULT_RESPONSE.
struct CreateOperationResultResponse
{
    static Hotline::MessageParser CreateParserWithCallback (
        std::function <void (CreateOperationResultResponse &, Hotline::SocketSession *)> &&callback);

    QueryId queryId_;
    ResourceId resourceId_;

    void Write (Hotline::SocketSession *session) const;
};

static_assert (std::is_pod_v <CreateOperationResultResponse>);

/// For message RESOURCE_IDS_RESPONSE.
struct IdsResponse
{
    static Hotline::MessageParser CreateParserWithCallback (
        std::function <void (IdsResponse &, Hotline::SocketSession *)> &&callback);

    QueryId queryId_;
    std::vector <ResourceId> ids_;

    void Write (Hotline::SocketSession *session) const;
};

/// For message GET_COLUMN_INFO_RESPONSE.
struct ColumnInfoResponse
{
    static Hotline::MessageParser CreateParserWithCallback (
        std::function <void (ColumnInfoResponse &, Hotline::SocketSession *)> &&callback);

    QueryId queryId_;
    Richard::DataType dataType_;
    std::string name_;

    void Write (Hotline::SocketSession *session) const;
};

/// For message GET_INDEX_INFO_RESPONSE.
struct IndexInfoResponse
{
    static Hotline::MessageParser CreateParserWithCallback (
        std::function <void (IndexInfoResponse &, Hotline::SocketSession *)> &&callback);

    QueryId queryId_;
    std::string name_;
    std::vector <ResourceId> columns_;

    void Write (Hotline::SocketSession *session) const;
};

/// For message SET_TABLE_NAME_REQUEST.
struct SetTableNameRequest
{
    static Hotline::MessageParser CreateParserWithCallback (
        std::function <void (SetTableNameRequest &, Hotline::SocketSession *)> &&callback);

    QueryId queryId_;
    ResourceId tableId_;
    std::string newName_;

    void Write (Hotline::SocketSession *session) const;
};

/// For message ADD_COLUMN_REQUEST.
struct AddColumnRequest
{
    static Hotline::MessageParser CreateParserWithCallback (
        std::function <void (AddColumnRequest &, Hotline::SocketSession *)> &&callback);

    QueryId queryId_;
    ResourceId tableId_;
    Richard::DataType dataType_;
    std::string name_;

    void Write (Hotline::SocketSession *session) const;
};

/// For message ADD_INDEX_REQUEST.
struct AddIndexRequest
{
    static Hotline::MessageParser CreateParserWithCallback (
        std::function <void (AddIndexRequest &, Hotline::SocketSession *)> &&callback);

    QueryId queryId_;
    ResourceId tableId_;
    std::string name_;
    std::vector <ResourceId> columns_;

    void Write (Hotline::SocketSession *session) const;
};

/// For message ADD_ROW_REQUEST.
struct AddRowRequest
{
    static Hotline::MessageParser CreateParserWithCallback (
        std::function <void (AddRowRequest &, Hotline::SocketSession *)> &&callback);

    QueryId queryId_;
    ResourceId tableId_;
    std::vector <std::pair <ResourceId, Richard::AnyDataContainer>> values_;

    void Write (Hotline::SocketSession *session) const;
};

/// For message CURSOR_ADVANCE_REQUEST.
struct CursorAdvanceRequest
{
    static Hotline::MessageParser CreateParserWithCallback (
        std::function <void (CursorAdvanceRequest &, Hotline::SocketSession *)> &&callback);

    QueryId queryId_;
    ResourceId cursorId_;
    uint64_t step_;

    void Write (Hotline::SocketSession *session) const;
};

/// For message CURSOR_GET_REQUEST.
struct CursorGetRequest
{
    static Hotline::MessageParser CreateParserWithCallback (
        std::function <void (CursorGetRequest &, Hotline::SocketSession *)> &&callback);

    QueryId queryId_;
    ResourceId cursorId_;
    ResourceId columnId_;

    void Write (Hotline::SocketSession *session) const;
};

/// For message CURSOR_GET_RESPONSE.
struct CursorGetResponse
{
    static Hotline::MessageParser CreateParserWithCallback (
        std::function <void (CursorGetResponse &, Hotline::SocketSession *)> &&callback);

    QueryId queryId_;
    Richard::AnyDataContainer value_;

    void Write (Hotline::SocketSession *session) const;
};

/// For message ADD_ROW_REQUEST.
struct CursorUpdateRequest
{
    static Hotline::MessageParser CreateParserWithCallback (
        std::function <void (CursorUpdateRequest &, Hotline::SocketSession *)> &&callback);

    QueryId queryId_;
    ResourceId cursorId_;
    std::vector <std::pair <ResourceId, Richard::AnyDataContainer>> values_;

    void Write (Hotline::SocketSession *session) const;
};

/// For messages CURSOR_DELETE_REQUEST and CLOSE_CURSOR_REQUEST.
struct CursorVoidActionRequest
{
    static Hotline::MessageParser CreateParserWithCallback (
        std::function <void (CursorVoidActionRequest &, Hotline::SocketSession *)> &&callback);

    QueryId queryId_;
    ResourceId cursorId_;

    void Write (Hotline::SocketSession *session) const;
};

/// For messages:
/// - GET_CONDUIT_READ_ACCESS_REQUEST.
/// - GET_CONDUIT_WRITE_ACCESS_REQUEST.
/// - CLOSE_CONDUIT_READ_ACCESS_REQUEST.
/// - CLOSE_CONDUIT_WRITE_ACCESS_REQUEST.
/// - GET_TABLE_IDS_REQUEST.
struct ConduitVoidActionRequest
{
    static Hotline::MessageParser CreateParserWithCallback (
        std::function <void (ConduitVoidActionRequest &, Hotline::SocketSession *)> &&callback);

    QueryId queryId_;

    void Write (Hotline::SocketSession *session) const;
};

/// For message ADD_TABLE_REQUEST.
struct AddTableRequest
{
    static Hotline::MessageParser CreateParserWithCallback (
        std::function <void (AddTableRequest &, Hotline::SocketSession *)> &&callback);

    QueryId queryId_;
    std::string tableName_;

    void Write (Hotline::SocketSession *session) const;
};

// TODO: Add message, that allows to capture multiple read/write guards.
}