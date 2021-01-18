#include <App/Miami/Messaging/Message.hpp>

namespace Miami::App::Messaging
{

// TODO: A lot of duplication and lack of readibility in parsers. Try to rewrite with
//       coroutines? Try to use nested parsers instead of macros?

namespace Utils
{
thread_local std::vector <Hotline::MemoryRegion> sharedRegionsMap;

thread_local std::vector <Richard::DataType> dataTypesCache;

template <typename Type, std::enable_if_t <std::is_pod_v <Type>, bool> = true>
void WritePODMessage (Type *value, Message messageType, Hotline::SocketSession *session)
{
    assert (value);
    assert (session);
    session->WriteMessage (static_cast <Hotline::MessageTypeId> (messageType), {value, sizeof (Type)});
}

#define START_WRITE_MAPPING          \
    assert (session);                \
    Utils::sharedRegionsMap.clear()

#define MAP_POD_WRITE(fieldName) \
    static_assert(std::is_pod_v<std::decay_t<decltype (fieldName)>>);                             \
    Utils::sharedRegionsMap.emplace_back(Hotline::MemoryRegion {&fieldName, sizeof (fieldName)})

#define MAP_POD_VECTOR_WRITE(fieldName)                                                                      \
    static_assert(std::is_pod_v<decltype (fieldName)::value_type>);                                          \
    std::size_t fieldName ## Size = fieldName.size();                                                        \
    MAP_POD_WRITE(fieldName ## Size);                                                                        \
                                                                                                             \
    if (fieldName ## Size)                                                                                   \
    {                                                                                                        \
        Utils::sharedRegionsMap.emplace_back(                                                                \
            Hotline::MemoryRegion {&fieldName[0], sizeof (fieldName[0]) * fieldName ## Size});               \
    }

#define MAP_ONE_TABLE_VALUE_INTERNAL(variable)                                                            \
    Richard::DataType type = variable.GetType ();                                                         \
    Utils::dataTypesCache.push_back (type);                                                               \
    MAP_POD_WRITE(Utils::dataTypesCache.back ());                                                         \
                                                                                                          \
    const void *data = variable.GetDataStartPointer ();                                                   \
    assert (data);                                                                                        \
    Utils::sharedRegionsMap.emplace_back (Hotline::MemoryRegion {data, Richard::GetDataTypeSize (type)})

// TODO: Works correctly only if there is only one table value in message.
#define MAP_ONE_TABLE_VALUE(field)              \
    Utils::dataTypesCache.clear ();             \
    Utils::dataTypesCache.reserve (1);          \
    MAP_ONE_TABLE_VALUE_INTERNAL(field)

#define MAP_TABLE_VALUES_WRITE(fieldName)                                                                     \
    std::size_t fieldName ## Size = fieldName.size();                                                         \
    MAP_POD_WRITE(fieldName ## Size);                                                                         \
    Utils::dataTypesCache.clear ();                                                                           \
    Utils::dataTypesCache.reserve (fieldName.size ());                                                        \
                                                                                                              \
    for (const auto &columnValuePair : fieldName)                                                             \
    {                                                                                                         \
        MAP_POD_WRITE(columnValuePair.first);                                                                 \
        MAP_ONE_TABLE_VALUE_INTERNAL(columnValuePair.second);                                                 \
    }

#define END_WRITE_MAPPING                                                                                \
    session->WriteMessage (static_cast <Hotline::MessageTypeId> (messageType), Utils::sharedRegionsMap)

#define VALIDATE_CHUNK_SIZE(expectedSize) \
    if (expectedSize != chunk.size ())    \
    {                                     \
        assert (false);                   \
        return {0, false};                \
    }

#define CREATE_POD_PARSER(MessageType)                                                                               \
    [finishCallback (std::move (callback)), firstCall (true)] (                                                      \
        const std::vector <uint8_t> &chunk, Hotline::SocketSession *session) mutable -> Hotline::MessageParserStatus \
    {                                                                                                                \
        static_assert (std::is_pod_v <MessageType>);                                                                 \
        if (firstCall)                                                                                               \
        {                                                                                                            \
            firstCall = false;                                                                                       \
            return {sizeof (MessageType), true};                                                                     \
        }                                                                                                            \
        else                                                                                                         \
        {                                                                                                            \
            VALIDATE_CHUNK_SIZE(sizeof (MessageType));                                                               \
            MessageType result;                                                                                      \
            memcpy (&result, &chunk[0], sizeof (MessageType));                                                       \
                                                                                                                     \
            if (finishCallback)                                                                                      \
            {                                                                                                        \
                finishCallback (result, session);                                                                    \
            }                                                                                                        \
                                                                                                                     \
            return {0, true};                                                                                        \
        }                                                                                                            \
    }

#define NEXT_STEP step = static_cast <Step> (static_cast <uint8_t> (step) + 1u)

#define REQUEST_POD(fieldPath)                              \
    static_assert (std::is_pod_v <decltype (fieldPath)>);   \
    return {sizeof (fieldPath), true}

#define READ_POD(fieldPath)                                          \
    static_assert (std::is_pod_v <decltype (fieldPath)>);            \
    VALIDATE_CHUNK_SIZE(sizeof (fieldPath));                  \
    memcpy (&fieldPath, &chunk[0], sizeof (fieldPath))

#define REQUEST_POD_VECTOR_SIZE return {sizeof (std::size_t), true};

#define READ_POD_VECTOR_SIZE(fieldPath)                                   \
    static_assert (std::is_pod_v<decltype (fieldPath)::value_type>);      \
    VALIDATE_CHUNK_SIZE(sizeof (std::size_t));                            \
    fieldPath.resize (*reinterpret_cast<const std::size_t *>(&chunk[0]));

#define REQUEST_POD_VECTOR_CONTENT(fieldPath)                                    \
    static_assert (std::is_pod_v<decltype (fieldPath)::value_type>);             \
    return {fieldPath.size () * sizeof (decltype (fieldPath)::value_type), true}

#define READ_POD_VECTOR_CONTENT(fieldPath)                           \
    static_assert (std::is_pod_v<decltype (fieldPath)::value_type>);                      \
    VALIDATE_CHUNK_SIZE((fieldPath.size () * sizeof (decltype (fieldPath)::value_type))); \
    memcpy (&fieldPath[0], &chunk[0], (fieldPath.size () * sizeof (decltype (fieldPath)::value_type)));

#define REQUEST_AND_READ_POD_VECTOR(fieldPath, sizeReaderStep, contentReaderStep, skipLabelName) \
        static_assert (std::is_pod_v<decltype (fieldPath)::value_type>);                         \
        NEXT_STEP;                                                                               \
        REQUEST_POD_VECTOR_SIZE;                                                                 \
    case sizeReaderStep:                                                                         \
        READ_POD_VECTOR_SIZE (fieldPath);                                                        \
        NEXT_STEP;                                                                               \
                                                                                                 \
        if (fieldPath.empty())                                                                   \
        {                                                                                        \
            goto skipLabelName;                                                                  \
        }                                                                                        \
        else                                                                                     \
        {                                                                                        \
            REQUEST_POD_VECTOR_CONTENT(fieldPath);                                               \
        }                                                                                        \
                                                                                                 \
    case contentReaderStep:                                                                      \
        READ_POD_VECTOR_CONTENT (fieldPath)                                                      \
        skipLabelName: ;

#define REQUEST_AND_READ_TABLE_VALUE(valuePath, typeReaderStep, contentReaderStep)               \
        NEXT_STEP;                                                                               \
        return {sizeof (Richard::DataType), true};                                               \
                                                                                                 \
    case typeReaderStep:                                                                         \
        VALIDATE_CHUNK_SIZE(sizeof (Richard::DataType));                                         \
        valuePath = Richard::AnyDataContainer (                                                  \
            *reinterpret_cast<const Richard::DataType *>(&chunk[0]));                            \
                                                                                                 \
        NEXT_STEP;                                                                               \
        return {Richard::GetDataTypeSize (valuePath.GetType ()), true};                          \
                                                                                                 \
    case contentReaderStep:                                                                      \
        VALIDATE_CHUNK_SIZE(Richard::GetDataTypeSize (valuePath.GetType ()));                    \
        {                                                                                        \
            void *data = valuePath.GetDataStartPointer ();                                       \
            if (data)                                                                            \
            {                                                                                    \
                memcpy (data, &chunk[0], Richard::GetDataTypeSize (valuePath.GetType ()));       \
            }                                                                                    \
            else                                                                                 \
            {                                                                                    \
                assert (false);                                                                  \
            }                                                                                    \
        }

#define REQUEST_AND_READ_TABLE_MAPPED_VALUE_VECTOR(fieldPath, counterPath, \
            countReaderStep, idReaderStep, typeReaderStep, dataReaderStep, \
            allReadLabelName, readNextLabelName)                                                           \
        NEXT_STEP;                                                                                         \
        return {sizeof (std::size_t), true};                                                               \
                                                                                                           \
    case countReaderStep:                                                                                  \
        VALIDATE_CHUNK_SIZE(sizeof (std::size_t));                                                         \
        fieldPath.resize (*reinterpret_cast <const std::size_t *> (&chunk[0]));                            \
                                                                                                           \
        if (fieldPath.empty ())                                                                            \
        {                                                                                                  \
            step = dataReaderStep;                                                                         \
            goto allReadLabelName;                                                                         \
        }                                                                                                  \
        else                                                                                               \
        {                                                                                                  \
            NEXT_STEP;                                                                                     \
            readNextLabelName: REQUEST_POD(fieldPath[counterPath].first);                                  \
        }                                                                                                  \
                                                                                                           \
    case idReaderStep:                                                                                     \
        READ_POD(fieldPath[counterPath].first);                                                            \
        REQUEST_AND_READ_TABLE_VALUE(fieldPath[counterPath].second, typeReaderStep, dataReaderStep);       \
        ++counterPath;                                                                                     \
                                                                                                           \
        if (counterPath < fieldPath.size ())                                                               \
        {                                                                                                  \
            step = idReaderStep;                                                                           \
            goto readNextLabelName;                                                                        \
        }                                                                                                  \
                                                                                                           \
    allReadLabelName: ;

#define CATCH_UNKNOWN_STEP default: return {0, false}
}

const char *GetMessageName (Message message)
{
    switch (message)
    {
        case Message::VOID_OPERATION_RESULT_RESPONSE:
            return "VOID_OPERATION_RESULT_RESPONSE";

        case Message::CREATE_OPERATION_RESULT_RESPONSE:
            return "CREATE_OPERATION_RESULT_RESPONSE";

        case Message::RESOURCE_IDS_RESPONSE:
            return "RESOURCE_IDS_RESPONSE";

        case Message::GET_TABLE_READ_ACCESS_REQUEST:
            return "GET_TABLE_READ_ACCESS_REQUEST";

        case Message::GET_TABLE_WRITE_ACCESS_REQUEST:
            return "GET_TABLE_WRITE_ACCESS_REQUEST";

        case Message::CLOSE_TABLE_READ_ACCESS_REQUEST:
            return "CLOSE_TABLE_READ_ACCESS_REQUEST";

        case Message::CLOSE_TABLE_WRITE_ACCESS_REQUEST:
            return "CLOSE_TABLE_WRITE_ACCESS_REQUEST";

        case Message::GET_TABLE_NAME_REQUEST:
            return "GET_TABLE_NAME_REQUEST";

        case Message::GET_TABLE_NAME_RESPONSE:
            return "GET_TABLE_NAME_RESPONSE";

        case Message::CREATE_READ_CURSOR_REQUEST:
            return "CREATE_READ_CURSOR_REQUEST";

        case Message::GET_COLUMNS_IDS_REQUEST:
            return "GET_COLUMNS_IDS_REQUEST";

        case Message::GET_COLUMN_INFO_REQUEST:
            return "GET_COLUMN_INFO_REQUEST";

        case Message::GET_COLUMN_INFO_RESPONSE:
            return "GET_COLUMN_INFO_RESPONSE";

        case Message::GET_INDICES_IDS_REQUEST:
            return "GET_INDICES_IDS_REQUEST";

        case Message::GET_INDEX_INFO_REQUEST:
            return "GET_INDEX_INFO_REQUEST";

        case Message::GET_INDEX_INFO_RESPONSE:
            return "GET_INDEX_INFO_RESPONSE";

        case Message::SET_TABLE_NAME_REQUEST:
            return "SET_TABLE_NAME_REQUEST";

        case Message::CREATE_EDIT_CURSOR_REQUEST:
            return "CREATE_EDIT_CURSOR_REQUEST";

        case Message::ADD_COLUMN_REQUEST:
            return "ADD_COLUMN_REQUEST";

        case Message::REMOVE_COLUMN_REQUEST:
            return "REMOVE_COLUMN_REQUEST";

        case Message::ADD_INDEX_REQUEST:
            return "ADD_INDEX_REQUEST";

        case Message::REMOVE_INDEX_REQUEST:
            return "REMOVE_INDEX_REQUEST";

        case Message::ADD_ROW_REQUEST:
            return "ADD_ROW_REQUEST";

        case Message::CURSOR_ADVANCE_REQUEST:
            return "CURSOR_ADVANCE_REQUEST";

        case Message::CURSOR_GET_REQUEST:
            return "CURSOR_GET_REQUEST";

        case Message::CURSOR_GET_RESPONSE:
            return "CURSOR_GET_RESPONSE";

        case Message::CURSOR_UPDATE_REQUEST:
            return "CURSOR_UPDATE_REQUEST";

        case Message::CURSOR_DELETE_REQUEST:
            return "CURSOR_DELETE_REQUEST";

        case Message::CLOSE_CURSOR_REQUEST:
            return "CLOSE_CURSOR_REQUEST";

        case Message::GET_CONDUIT_READ_ACCESS_REQUEST:
            return "GET_CONDUIT_READ_ACCESS_REQUEST";

        case Message::GET_CONDUIT_WRITE_ACCESS_REQUEST:
            return "GET_CONDUIT_WRITE_ACCESS_REQUEST";

        case Message::CLOSE_CONDUIT_READ_ACCESS_REQUEST:
            return "CLOSE_CONDUIT_READ_ACCESS_REQUEST";

        case Message::CLOSE_CONDUIT_WRITE_ACCESS_REQUEST:
            return "CLOSE_CONDUIT_WRITE_ACCESS_REQUEST";

        case Message::GET_TABLE_IDS_REQUEST:
            return "GET_TABLE_IDS_REQUEST";

        case Message::ADD_TABLE_REQUEST:
            return "ADD_TABLE_REQUEST";

        case Message::REMOVE_TABLE_REQUEST:
            return "REMOVE_TABLE_REQUEST";
    }

    assert (false);
    return "UNKNOWN";
}

const char *GetOperationResultName (OperationResult operationResult)
{
    switch (operationResult)
    {
        case OperationResult::OK:
            return "OK";

        case OperationResult::INTERNAL_ERROR:
            return "INTERNAL_ERROR";

        case OperationResult::ALREADY_HAS_WRITE_ACCESS:
            return "ALREADY_HAS_WRITE_ACCESS";

        case OperationResult::ALREADY_HAS_READ_ACCESS:
            return "ALREADY_HAS_READ_ACCESS";

        case OperationResult::CONDUIT_READ_ACCESS_REQUIRED:
            return "CONDUIT_READ_ACCESS_REQUIRED";

        case OperationResult::CONDUIT_READ_OR_WRITE_ACCESS_REQUIRED:
            return "CONDUIT_READ_OR_WRITE_ACCESS_REQUIRED";

        case OperationResult::CONDUIT_WRITE_ACCESS_REQUIRED:
            return "CONDUIT_WRITE_ACCESS_REQUIRED";

        case OperationResult::TABLE_READ_ACCESS_REQUIRED:
            return "TABLE_READ_ACCESS_REQUIRED";

        case OperationResult::TABLE_READ_OR_WRITE_ACCESS_REQUIRED:
            return "TABLE_READ_OR_WRITE_ACCESS_REQUIRED";

        case OperationResult::TABLE_WRITE_ACCESS_REQUIRED:
            return "TABLE_WRITE_ACCESS_REQUIRED";

        case OperationResult::CURSOR_WITH_GIVEN_ID_NOT_FOUND:
            return "CURSOR_WITH_GIVEN_ID_NOT_FOUND";

        case OperationResult::NULL_COLUMN_VALUE:
            return "NULL_COLUMN_VALUE";

        case OperationResult::CURSOR_ADVANCE_STOPPED_AT_BEGIN:
            return "CURSOR_ADVANCE_STOPPED_AT_BEGIN";

        case OperationResult::CURSOR_ADVANCE_STOPPED_AT_END:
            return "CURSOR_ADVANCE_STOPPED_AT_END";

        case OperationResult::CURSOR_GET_CURRENT_UNABLE_TO_GET_FROM_END:
            return "CURSOR_GET_CURRENT_UNABLE_TO_GET_FROM_END";

        case OperationResult::COLUMN_WITH_GIVEN_ID_NOT_FOUND:
            return "COLUMN_WITH_GIVEN_ID_NOT_FOUND";

        case OperationResult::INDEX_WITH_GIVEN_ID_NOT_FOUND:
            return "INDEX_WITH_GIVEN_ID_NOT_FOUND";

        case OperationResult::ROW_WITH_GIVEN_ID_NOT_FOUND:
            return "ROW_WITH_GIVEN_ID_NOT_FOUND";

        case OperationResult::TABLE_WITH_GIVEN_ID_NOT_FOUND:
            return "TABLE_WITH_GIVEN_ID_NOT_FOUND";

        case OperationResult::TABLE_NAME_SHOULD_NOT_BE_EMPTY:
            return "TABLE_NAME_SHOULD_NOT_BE_EMPTY";

        case OperationResult::COLUMN_NAME_SHOULD_NOT_BE_EMPTY:
            return "COLUMN_NAME_SHOULD_NOT_BE_EMPTY";

        case OperationResult::INDEX_NAME_SHOULD_NOT_BE_EMPTY:
            return "INDEX_NAME_SHOULD_NOT_BE_EMPTY";

        case OperationResult::INDEX_MUST_DEPEND_ON_AT_LEAST_ONE_COLUMN:
            return "INDEX_MUST_DEPEND_ON_AT_LEAST_ONE_COLUMN";

        case OperationResult::COLUMN_REMOVAL_BLOCKED_BY_DEPENDANT_INDEX:
            return "COLUMN_REMOVAL_BLOCKED_BY_DEPENDANT_INDEX";

        case OperationResult::INDEX_REMOVAL_BLOCKED_BY_DEPENDANT_CURSORS:
            return "INDEX_REMOVAL_BLOCKED_BY_DEPENDANT_CURSORS";

        case OperationResult::TABLE_REMOVAL_BLOCKED:
            return "TABLE_REMOVAL_BLOCKED";

        case OperationResult::NEW_COLUMN_VALUE_TYPE_MISMATCH:
            return "NEW_COLUMN_VALUE_TYPE_MISMATCH";

        case OperationResult::DUPLICATE_COLUMN_VALUES_IN_INSERTION_REQUEST:
            return "DUPLICATE_COLUMN_VALUES_IN_INSERTION_REQUEST";
    }

    assert (false);
    return "UNKNOWN";
}

Hotline::MessageParser VoidOperationResultResponse::CreateParserWithCallback (
    std::function <void (VoidOperationResultResponse &, Hotline::SocketSession *)> &&callback)
{
    assert (callback);
    return CREATE_POD_PARSER(VoidOperationResultResponse);
}

void VoidOperationResultResponse::Write (Message messageType, Hotline::SocketSession *session) const
{
    Utils::WritePODMessage (this, messageType, session);
}

Hotline::MessageParser TableOperationRequest::CreateParserWithCallback (
    std::function <void (TableOperationRequest &, Hotline::SocketSession *)> &&callback)
{
    assert (callback);
    return CREATE_POD_PARSER(TableOperationRequest);
}

void TableOperationRequest::Write (Message messageType, Hotline::SocketSession *session) const
{
    Utils::WritePODMessage (this, messageType, session);
}

Hotline::MessageParser GetTableNameResponse::CreateParserWithCallback (
    std::function <void (GetTableNameResponse &, Hotline::SocketSession *)> &&callback)
{
    assert (callback);
    enum Step : uint8_t
    {
        START = 0,
        READ_QUERY_ID,
        READ_TABLE_NAME_SIZE,
        READ_TABLE_NAME_CONTENT
    };

    return [finishCallback (std::move (callback)),
        step (Step::START),
        result (GetTableNameResponse {})]

        (const std::vector <uint8_t> &chunk,
         Hotline::SocketSession *session) mutable -> Hotline::MessageParserStatus
    {
        switch (step)
        {
            case START:
                NEXT_STEP;
                REQUEST_POD (result.queryId_);

            case READ_QUERY_ID:
            READ_POD (result.queryId_);
                REQUEST_AND_READ_POD_VECTOR(result.tableName_, READ_TABLE_NAME_SIZE,
                                            READ_TABLE_NAME_CONTENT, GetTableNameResponse_NAME_READ_SKIP_LABEL);

                if (finishCallback)
                {
                    finishCallback (result, session);
                }

                return {0, true};

            CATCH_UNKNOWN_STEP;
        }
    };
}

void GetTableNameResponse::Write (Message messageType, Hotline::SocketSession *session) const
{
    START_WRITE_MAPPING;
    MAP_POD_WRITE(queryId_);
    MAP_POD_VECTOR_WRITE(tableName_);
    END_WRITE_MAPPING;
}

Hotline::MessageParser TablePartOperationRequest::CreateParserWithCallback (
    std::function <void (TablePartOperationRequest &, Hotline::SocketSession *)> &&callback)
{
    assert (callback);
    return CREATE_POD_PARSER(TablePartOperationRequest);
}

void TablePartOperationRequest::Write (Message messageType, Hotline::SocketSession *session) const
{
    Utils::WritePODMessage (this, messageType, session);
}

Hotline::MessageParser CreateOperationResultResponse::CreateParserWithCallback (
    std::function <void (CreateOperationResultResponse &, Hotline::SocketSession *)> &&callback)
{
    assert (callback);
    return CREATE_POD_PARSER(CreateOperationResultResponse);
}

void CreateOperationResultResponse::Write (Message messageType, Hotline::SocketSession *session) const
{
    Utils::WritePODMessage (this, messageType, session);
}

Hotline::MessageParser
IdsResponse::CreateParserWithCallback (std::function <void (IdsResponse &, Hotline::SocketSession *)> &&callback)
{
    assert (callback);
    enum Step : uint8_t
    {
        START = 0,
        READ_QUERY_ID,
        READ_IDS_COUNT,
        READ_IDS
    };

    return [finishCallback (std::move (callback)),
        step (Step::START),
        result (IdsResponse {})]

        (const std::vector <uint8_t> &chunk,
         Hotline::SocketSession *session) mutable -> Hotline::MessageParserStatus
    {
        switch (step)
        {
            case START:
                NEXT_STEP;
                REQUEST_POD (result.queryId_);

            case READ_QUERY_ID:
            READ_POD (result.queryId_);
                REQUEST_AND_READ_POD_VECTOR(result.ids_, READ_IDS_COUNT,
                                            READ_IDS, IdsResponse_IDS_READ_SKIP_LABEL);

                if (finishCallback)
                {
                    finishCallback (result, session);
                }

                return {0, true};

            CATCH_UNKNOWN_STEP;
        }
    };
}

void IdsResponse::Write (Message messageType, Hotline::SocketSession *session) const
{
    START_WRITE_MAPPING;
    MAP_POD_WRITE(queryId_);
    MAP_POD_VECTOR_WRITE(ids_);
    END_WRITE_MAPPING;
}

Hotline::MessageParser ColumnInfoResponse::CreateParserWithCallback (
    std::function <void (ColumnInfoResponse &, Hotline::SocketSession *)> &&callback)
{
    assert (callback);
    enum Step : uint8_t
    {
        START = 0,
        READ_QUERY_ID,
        READ_DATA_TYPE,
        READ_NAME_SIZE,
        READ_NAME_CONTENT
    };

    return [finishCallback (std::move (callback)),
        step (Step::START),
        result (ColumnInfoResponse {})]

        (const std::vector <uint8_t> &chunk,
         Hotline::SocketSession *session) mutable -> Hotline::MessageParserStatus
    {
        switch (step)
        {
            case START:
                NEXT_STEP;
                REQUEST_POD (result.queryId_);

            case READ_QUERY_ID:
            READ_POD (result.queryId_);
                NEXT_STEP;
                REQUEST_POD (result.dataType_);

            case READ_DATA_TYPE:
            READ_POD (result.dataType_);
                REQUEST_AND_READ_POD_VECTOR(result.name_, READ_NAME_SIZE,
                                            READ_NAME_CONTENT, ColumnInfoResponse_NAME_READ_SKIP_LABEL);

                if (finishCallback)
                {
                    finishCallback (result, session);
                }

                return {0, true};

            CATCH_UNKNOWN_STEP;
        }
    };
}

void ColumnInfoResponse::Write (Message messageType, Hotline::SocketSession *session) const
{
    START_WRITE_MAPPING;
    MAP_POD_WRITE(queryId_);
    MAP_POD_WRITE(dataType_);
    MAP_POD_VECTOR_WRITE(name_);
    END_WRITE_MAPPING;
}

Hotline::MessageParser IndexInfoResponse::CreateParserWithCallback (
    std::function <void (IndexInfoResponse &, Hotline::SocketSession *)> &&callback)
{
    assert (callback);
    enum Step : uint8_t
    {
        START = 0,
        READ_QUERY_ID,
        READ_NAME_SIZE,
        READ_NAME_CONTENT,
        READ_COLUMNS_COUNT,
        READ_COLUMNS
    };

    return [finishCallback (std::move (callback)),
        step (Step::START),
        result (IndexInfoResponse {})]

        (const std::vector <uint8_t> &chunk,
         Hotline::SocketSession *session) mutable -> Hotline::MessageParserStatus
    {
        switch (step)
        {
            case START:
                NEXT_STEP;
                REQUEST_POD (result.queryId_);

            case READ_QUERY_ID:
            READ_POD (result.queryId_);
                REQUEST_AND_READ_POD_VECTOR(result.name_, READ_NAME_SIZE,
                                            READ_NAME_CONTENT, IndexInfoResponse_NAME_READ_SKIP_LABEL);

                REQUEST_AND_READ_POD_VECTOR(result.columns_, READ_COLUMNS_COUNT,
                                            READ_COLUMNS, IndexInfoResponse_COLUMNS_READ_SKIP_LABEL);

                if (finishCallback)
                {
                    finishCallback (result, session);
                }

                return {0, true};

            CATCH_UNKNOWN_STEP;
        }
    };
}

void IndexInfoResponse::Write (Message messageType, Hotline::SocketSession *session) const
{
    START_WRITE_MAPPING;
    MAP_POD_WRITE(queryId_);
    MAP_POD_VECTOR_WRITE(name_);
    MAP_POD_VECTOR_WRITE(columns_);
    END_WRITE_MAPPING;
}

Hotline::MessageParser SetTableNameRequest::CreateParserWithCallback (
    std::function <void (SetTableNameRequest &, Hotline::SocketSession *)> &&callback)
{
    assert (callback);
    enum Step : uint8_t
    {
        START = 0,
        READ_QUERY_ID,
        READ_TABLE_ID,
        READ_NEW_NAME_SIZE,
        READ_NEW_NAME_CONTENT
    };

    return [finishCallback (std::move (callback)),
        step (Step::START),
        result (SetTableNameRequest {})]

        (const std::vector <uint8_t> &chunk,
         Hotline::SocketSession *session) mutable -> Hotline::MessageParserStatus
    {
        switch (step)
        {
            case START:
                NEXT_STEP;
                REQUEST_POD (result.queryId_);

            case READ_QUERY_ID:
            READ_POD (result.queryId_);
                NEXT_STEP;
                REQUEST_POD (result.tableId_);

            case READ_TABLE_ID:
            READ_POD (result.tableId_);
                REQUEST_AND_READ_POD_VECTOR(result.newName_, READ_NEW_NAME_SIZE,
                                            READ_NEW_NAME_CONTENT, SetTableNameRequest_NAME_READ_SKIP_LABEL);

                if (finishCallback)
                {
                    finishCallback (result, session);
                }

                return {0, true};

            CATCH_UNKNOWN_STEP;
        }
    };
}

void SetTableNameRequest::Write (Message messageType, Hotline::SocketSession *session) const
{
    START_WRITE_MAPPING;
    MAP_POD_WRITE(queryId_);
    MAP_POD_WRITE(tableId_);
    MAP_POD_VECTOR_WRITE(newName_);
    END_WRITE_MAPPING;
}

Hotline::MessageParser AddColumnRequest::CreateParserWithCallback (
    std::function <void (AddColumnRequest &, Hotline::SocketSession *)> &&callback)
{
    assert (callback);
    enum Step : uint8_t
    {
        START = 0,
        READ_QUERY_ID,
        READ_TABLE_ID,
        READ_DATA_TYPE,
        READ_NAME_SIZE,
        READ_NAME_CONTENT
    };

    return [finishCallback (std::move (callback)),
        step (Step::START),
        result (AddColumnRequest {})]

        (const std::vector <uint8_t> &chunk,
         Hotline::SocketSession *session) mutable -> Hotline::MessageParserStatus
    {
        switch (step)
        {
            case START:
                NEXT_STEP;
                REQUEST_POD (result.queryId_);

            case READ_QUERY_ID:
            READ_POD (result.queryId_);
                NEXT_STEP;
                REQUEST_POD (result.tableId_);

            case READ_TABLE_ID:
            READ_POD (result.tableId_);
                NEXT_STEP;
                REQUEST_POD (result.dataType_);

            case READ_DATA_TYPE:
            READ_POD (result.dataType_);
                REQUEST_AND_READ_POD_VECTOR(result.name_, READ_NAME_SIZE,
                                            READ_NAME_CONTENT, AddColumnRequest_NAME_READ_SKIP_LABEL);

                if (finishCallback)
                {
                    finishCallback (result, session);
                }

                return {0, true};

            CATCH_UNKNOWN_STEP;
        }
    };
}

void AddColumnRequest::Write (Message messageType, Hotline::SocketSession *session) const
{
    START_WRITE_MAPPING;
    MAP_POD_WRITE(queryId_);
    MAP_POD_WRITE(tableId_);
    MAP_POD_WRITE(dataType_);
    MAP_POD_VECTOR_WRITE(name_);
    END_WRITE_MAPPING;
}

Hotline::MessageParser AddIndexRequest::CreateParserWithCallback (
    std::function <void (AddIndexRequest &, Hotline::SocketSession *)> &&callback)
{
    assert (callback);
    enum Step : uint8_t
    {
        START = 0,
        READ_QUERY_ID,
        READ_TABLE_ID,
        READ_NAME_SIZE,
        READ_NAME_CONTENT,
        READ_COLUMNS_COUNT,
        READ_COLUMNS
    };

    return [finishCallback (std::move (callback)),
        step (Step::START),
        result (AddIndexRequest {})]

        (const std::vector <uint8_t> &chunk,
         Hotline::SocketSession *session) mutable -> Hotline::MessageParserStatus
    {
        switch (step)
        {
            case START:
                NEXT_STEP;
                REQUEST_POD (result.queryId_);

            case READ_QUERY_ID:
            READ_POD (result.queryId_);
                NEXT_STEP;
                REQUEST_POD (result.tableId_);

            case READ_TABLE_ID:
            READ_POD (result.tableId_);
                REQUEST_AND_READ_POD_VECTOR(result.name_, READ_NAME_SIZE,
                                            READ_NAME_CONTENT, AddIndexRequest_NAME_READ_SKIP_LABEL);

                REQUEST_AND_READ_POD_VECTOR(result.columns_, READ_COLUMNS_COUNT,
                                            READ_COLUMNS, AddIndexRequest_COLUMNS_READ_SKIP_LABEL);

                if (finishCallback)
                {
                    finishCallback (result, session);
                }

                return {0, true};

            CATCH_UNKNOWN_STEP;
        }
    };
}

void AddIndexRequest::Write (Message messageType, Hotline::SocketSession *session) const
{
    START_WRITE_MAPPING;
    MAP_POD_WRITE(queryId_);
    MAP_POD_WRITE(tableId_);
    MAP_POD_VECTOR_WRITE(name_);
    MAP_POD_VECTOR_WRITE(columns_);
    END_WRITE_MAPPING;
}

Hotline::MessageParser
AddRowRequest::CreateParserWithCallback (std::function <void (AddRowRequest &, Hotline::SocketSession *)> &&callback)
{
    assert (callback);
    enum Step : uint8_t
    {
        START = 0,
        READ_QUERY_ID,
        READ_TABLE_ID,
        READ_VALUES_COUNT,
        READ_COLUMN_ID,
        READ_DATA_TYPE,
        READ_DATA
    };

    return [finishCallback (std::move (callback)),
        step (Step::START),

        // TODO: Adhok, because AnyDataContainer is not copyable.
        result (std::make_shared <AddRowRequest> ()),
        valuesRead (std::size_t (0u))]

        (const std::vector <uint8_t> &chunk,
         Hotline::SocketSession *session) mutable -> Hotline::MessageParserStatus
    {
        switch (step)
        {
            case START:
                NEXT_STEP;
                REQUEST_POD (result->queryId_);

            case READ_QUERY_ID:
            READ_POD (result->queryId_);
                NEXT_STEP;
                REQUEST_POD (result->tableId_);

            case READ_TABLE_ID:
            READ_POD (result->tableId_);
                REQUEST_AND_READ_TABLE_MAPPED_VALUE_VECTOR(
                    result->values_, valuesRead, READ_VALUES_COUNT, READ_COLUMN_ID, READ_DATA_TYPE, READ_DATA,
                    CreateParserWithCallback_AllValuesRead, CreateParserWithCallback_ReadNextColumnId);

                if (finishCallback)
                {
                    finishCallback (*result, session);
                }

                return {0, true};

            CATCH_UNKNOWN_STEP;
        }
    };
}

void AddRowRequest::Write (Message messageType, Hotline::SocketSession *session) const
{
    START_WRITE_MAPPING;
    MAP_POD_WRITE(queryId_);
    MAP_POD_WRITE(tableId_);
    MAP_TABLE_VALUES_WRITE(values_);
    END_WRITE_MAPPING;
}

Hotline::MessageParser CursorAdvanceRequest::CreateParserWithCallback (
    std::function <void (CursorAdvanceRequest &, Hotline::SocketSession *)> &&callback)
{
    assert (callback);
    return CREATE_POD_PARSER(CursorAdvanceRequest);
}

void CursorAdvanceRequest::Write (Message messageType, Hotline::SocketSession *session) const
{
    Utils::WritePODMessage (this, messageType, session);
}

Hotline::MessageParser CursorGetRequest::CreateParserWithCallback (
    std::function <void (CursorGetRequest &, Hotline::SocketSession *)> &&callback)
{
    assert (callback);
    return CREATE_POD_PARSER(CursorGetRequest);
}

void CursorGetRequest::Write (Message messageType, Hotline::SocketSession *session) const
{
    Utils::WritePODMessage (this, messageType, session);
}

Hotline::MessageParser CursorGetResponse::CreateParserWithCallback (
    std::function <void (CursorGetResponse &, Hotline::SocketSession *)> &&callback)
{
    assert (callback);
    enum Step : uint8_t
    {
        START = 0,
        READ_QUERY_ID,
        READ_DATA_TYPE,
        READ_DATA
    };

    return [finishCallback (std::move (callback)),
        step (Step::START),

        // TODO: Adhok, because AnyDataContainer is not copyable.
        result (std::make_shared <CursorGetResponse> ())]

        (const std::vector <uint8_t> &chunk,
         Hotline::SocketSession *session) mutable -> Hotline::MessageParserStatus
    {
        switch (step)
        {
            case START:
                NEXT_STEP;
                REQUEST_POD (result->queryId_);

            case READ_QUERY_ID:
            READ_POD (result->queryId_);
                REQUEST_AND_READ_TABLE_VALUE(result->value_, READ_DATA_TYPE, READ_DATA);

                if (finishCallback)
                {
                    finishCallback (*result, session);
                }

                return {0, true};

            CATCH_UNKNOWN_STEP;
        }
    };
}

void CursorGetResponse::Write (Message messageType, Hotline::SocketSession *session) const
{
    START_WRITE_MAPPING;
    MAP_POD_WRITE(queryId_);
    MAP_ONE_TABLE_VALUE(value_);
    END_WRITE_MAPPING;
}

Hotline::MessageParser CursorUpdateRequest::CreateParserWithCallback (
    std::function <void (CursorUpdateRequest &, Hotline::SocketSession *)> &&callback)
{
    assert (callback);
    enum Step : uint8_t
    {
        START = 0,
        READ_QUERY_ID,
        READ_CURSOR_ID,
        READ_VALUES_COUNT,
        READ_COLUMN_ID,
        READ_DATA_TYPE,
        READ_DATA
    };

    return [finishCallback (std::move (callback)),
        step (Step::START),

        // TODO: Adhok, because AnyDataContainer is not copyable.
        result (std::make_shared <CursorUpdateRequest> ()),
        valuesRead (std::size_t (0u))]

        (const std::vector <uint8_t> &chunk,
         Hotline::SocketSession *session) mutable -> Hotline::MessageParserStatus
    {
        switch (step)
        {
            case START:
                NEXT_STEP;
                REQUEST_POD (result->queryId_);

            case READ_QUERY_ID:
            READ_POD (result->queryId_);
                NEXT_STEP;
                REQUEST_POD (result->cursorId_);

            case READ_CURSOR_ID:
            READ_POD (result->cursorId_);
                REQUEST_AND_READ_TABLE_MAPPED_VALUE_VECTOR(
                    result->values_, valuesRead, READ_VALUES_COUNT, READ_COLUMN_ID, READ_DATA_TYPE, READ_DATA,
                    CreateParserWithCallback_AllValuesRead, CreateParserWithCallback_ReadNextColumnId);

                if (finishCallback)
                {
                    finishCallback (*result, session);
                }

                return {0, true};

            CATCH_UNKNOWN_STEP;
        }
    };
}

void CursorUpdateRequest::Write (Message messageType, Hotline::SocketSession *session) const
{
    START_WRITE_MAPPING;
    MAP_POD_WRITE(queryId_);
    MAP_POD_WRITE(cursorId_);
    MAP_TABLE_VALUES_WRITE(values_);
    END_WRITE_MAPPING;
}

Hotline::MessageParser CursorVoidActionRequest::CreateParserWithCallback (
    std::function <void (CursorVoidActionRequest &, Hotline::SocketSession *)> &&callback)
{
    assert (callback);
    return CREATE_POD_PARSER(CursorVoidActionRequest);
}

void CursorVoidActionRequest::Write (Message messageType, Hotline::SocketSession *session) const
{
    Utils::WritePODMessage (this, messageType, session);
}

Hotline::MessageParser ConduitVoidActionRequest::CreateParserWithCallback (
    std::function <void (ConduitVoidActionRequest &, Hotline::SocketSession *)> &&callback)
{
    assert (callback);
    return CREATE_POD_PARSER(ConduitVoidActionRequest);
}

void ConduitVoidActionRequest::Write (Message messageType, Hotline::SocketSession *session) const
{
    Utils::WritePODMessage (this, messageType, session);
}

Hotline::MessageParser AddTableRequest::CreateParserWithCallback (
    std::function <void (AddTableRequest &, Hotline::SocketSession *)> &&callback)
{
    assert (callback);
    enum Step : uint8_t
    {
        START = 0,
        READ_QUERY_ID,
        READ_TABLE_NAME_SIZE,
        READ_TABLE_NAME_CONTENT
    };

    return [finishCallback (std::move (callback)),
        step (Step::START),
        result (AddTableRequest {})]

        (const std::vector <uint8_t> &chunk,
         Hotline::SocketSession *session) mutable -> Hotline::MessageParserStatus
    {
        switch (step)
        {
            case START:
                NEXT_STEP;
                REQUEST_POD (result.queryId_);

            case READ_QUERY_ID:
            READ_POD (result.queryId_);
                REQUEST_AND_READ_POD_VECTOR(result.tableName_, READ_TABLE_NAME_SIZE,
                                            READ_TABLE_NAME_CONTENT, AddTableRequest_NAME_READ_SKIP_LABEL);

                if (finishCallback)
                {
                    finishCallback (result, session);
                }

                return {0, true};

            CATCH_UNKNOWN_STEP;
        }
    };
}

void AddTableRequest::Write (Message messageType, Hotline::SocketSession *session) const
{
    START_WRITE_MAPPING;
    MAP_POD_WRITE(queryId_);
    MAP_POD_VECTOR_WRITE(tableName_);
    END_WRITE_MAPPING;
}
}
