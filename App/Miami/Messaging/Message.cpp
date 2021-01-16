#include <App/Miami/Messaging/Message.hpp>

namespace Miami::App::Messaging
{
namespace Utils
{
thread_local std::vector <Hotline::MemoryRegion> sharedRegionsMap;

thread_local std::vector <Richard::DataType> dataTypesCache;

template <typename Type, std::enable_if_t <std::is_pod_v <Type>, bool> = true>
void WritePODMessage (Type *value, Hotline::SocketSession *session)
{
    assert (value);
    assert (session);
    session->Write ({value, sizeof (Type)});
}

#define START_WRITE_MAPPING          \
    assert (session);                \
    Utils::sharedRegionsMap.clear()

#define MAP_POD_WRITE(fieldName) \
    static_assert(std::is_pod_v<std::decay_t<decltype (fieldName)>>);                             \
    Utils::sharedRegionsMap.emplace_back(Hotline::MemoryRegion {&fieldName, sizeof (fieldName)})

#define MAP_STRING_WRITE(fieldName)                                                                      \
    std::size_t fieldName ## Length = fieldName.length();                                                \
    MAP_POD_WRITE(fieldName ## Length);                                                                  \
    Utils::sharedRegionsMap.emplace_back(Hotline::MemoryRegion {fieldName.c_str(), fieldName ## Length})

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

#define MAP_ONE_TABLE_VALUE(field)              \
    Utils::dataTypesCache.clear ();             \
    Utils::dataTypesCache.reserve (1);          \
    MAP_ONE_TABLE_VALUE_INTERNAL(field)

#define MAP_TABLE_VALUES_WRITE(fieldName)                                                                     \
    Utils::dataTypesCache.clear ();                                                                           \
    Utils::dataTypesCache.reserve (fieldName.size ());                                                        \
                                                                                                              \
    for (const auto &columnValuePair : fieldName)                                                             \
    {                                                                                                         \
        MAP_POD_WRITE(columnValuePair.first);                                                                 \
        MAP_ONE_TABLE_VALUE_INTERNAL(columnValuePair.second);                                                 \
    }

#define END_WRITE_MAPPING session->Write (Utils::sharedRegionsMap)
}

void VoidOperationResultResponse::Write (Hotline::SocketSession *session) const
{
    Utils::WritePODMessage (this, session);
}

void TableOperationRequest::Write (Hotline::SocketSession *session) const
{
    Utils::WritePODMessage (this, session);
}

void GetTableNameResponse::Write (Hotline::SocketSession *session) const
{
    START_WRITE_MAPPING;
    MAP_POD_WRITE(queryId_);
    MAP_STRING_WRITE(tableName_);
    END_WRITE_MAPPING;
}

void TablePartOperationRequest::Write (Hotline::SocketSession *session) const
{
    Utils::WritePODMessage (this, session);
}

void CreateOperationResultResponse::Write (Hotline::SocketSession *session) const
{
    Utils::WritePODMessage (this, session);
}

void IdsResponse::Write (Hotline::SocketSession *session) const
{
    START_WRITE_MAPPING;
    MAP_POD_VECTOR_WRITE(ids_);
    END_WRITE_MAPPING;
}

void ColumnInfoResponse::Write (Hotline::SocketSession *session) const
{
    START_WRITE_MAPPING;
    MAP_POD_WRITE(queryId_);
    MAP_POD_WRITE(dataType_);
    MAP_STRING_WRITE(name_);
    END_WRITE_MAPPING;
}

void IndexInfoResponse::Write (Hotline::SocketSession *session) const
{
    START_WRITE_MAPPING;
    MAP_POD_WRITE(queryId_);
    MAP_STRING_WRITE(name_);
    MAP_POD_VECTOR_WRITE(columns_);
    END_WRITE_MAPPING;
}

void SetTableNameRequest::Write (Hotline::SocketSession *session) const
{
    START_WRITE_MAPPING;
    MAP_POD_WRITE(queryId_);
    MAP_POD_WRITE(tableId_);
    MAP_STRING_WRITE(newName_);
    END_WRITE_MAPPING;
}

void AddColumnRequest::Write (Hotline::SocketSession *session) const
{
    START_WRITE_MAPPING;
    MAP_POD_WRITE(queryId_);
    MAP_POD_WRITE(tableId_);
    MAP_POD_WRITE(dataType_);
    MAP_STRING_WRITE(name_);
    END_WRITE_MAPPING;
}

void AddIndexRequest::Write (Hotline::SocketSession *session) const
{
    START_WRITE_MAPPING;
    MAP_POD_WRITE(queryId_);
    MAP_POD_WRITE(tableId_);
    MAP_STRING_WRITE(name_);
    MAP_POD_VECTOR_WRITE(columns_);
    END_WRITE_MAPPING;
}

void AddRowRequest::Write (Hotline::SocketSession *session) const
{
    START_WRITE_MAPPING;
    MAP_POD_WRITE(queryId_);
    MAP_POD_WRITE(tableId_);
    MAP_TABLE_VALUES_WRITE(values_);
    END_WRITE_MAPPING;
}

void CursorAdvanceRequest::Write (Hotline::SocketSession *session) const
{
    Utils::WritePODMessage (this, session);
}

void CursorGetRequest::Write (Hotline::SocketSession *session) const
{
    Utils::WritePODMessage (this, session);
}

void CursorGetResponse::Write (Hotline::SocketSession *session) const
{
    START_WRITE_MAPPING;
    MAP_POD_WRITE(queryId_);
    MAP_ONE_TABLE_VALUE(value_);
    END_WRITE_MAPPING;
}

void CursorUpdateRequest::Write (Hotline::SocketSession *session) const
{
    START_WRITE_MAPPING;
    MAP_POD_WRITE(queryId_);
    MAP_POD_WRITE(cursorId_);
    MAP_TABLE_VALUES_WRITE(values_);
    END_WRITE_MAPPING;
}

void CursorVoidActionRequest::Write (Hotline::SocketSession *session) const
{
    Utils::WritePODMessage (this, session);
}

void ConduitVoidActionRequest::Write (Hotline::SocketSession *session) const
{
    Utils::WritePODMessage (this, session);
}

void AddTableRequest::Write (Hotline::SocketSession *session) const
{
    START_WRITE_MAPPING;
    MAP_POD_WRITE(queryId_);
    MAP_STRING_WRITE(tableName_);
    END_WRITE_MAPPING;
}
}
