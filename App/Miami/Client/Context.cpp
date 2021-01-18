// Not only we don't need GDI, but it also has ERROR macro that breaks Evan's LogLevel.
#define NOGDI

#include <iostream>

#include <App/Miami/Client/Context.hpp>
#include <App/Miami/Messaging/Message.hpp>

#include <Miami/Evan/Logger.hpp>

namespace Miami::App::Client
{
Context::Context ()
    : multithreadingContext_ (1), // We don't really need Disco, so minimum worker thread count specified.
      socketClient_ (&multithreadingContext_),
      clientDelayedOutput_ (),
      clientDelayedOutputGuard_ ()
{
    Evan::Logger::Get ().Log (Evan::LogLevel::INFO, "Client context initialized.");
    RegisterMessages ();
}

ResultCode Context::Connect (const std::string &host, const std::string &service)
{
    Hotline::ResultCode socketResult = socketClient_.Start (host, service);
    if (socketResult == Hotline::ResultCode::OK)
    {
        return ResultCode::OK;
    }
    else
    {
        Evan::Logger::Get ().Log (
            Evan::LogLevel::ERROR, "Caught Hotline error during socket client connect " +
                                   std::to_string (static_cast <uint32_t> (socketResult)) + "!");
        return ResultCode::UNABLE_TO_CONNECT;
    }
}

Hotline::SocketClient &Context::Client ()
{
    return socketClient_;
}

void Context::PrintAllDelayedOutput ()
{
    std::unique_lock <std::mutex> lock (clientDelayedOutputGuard_);
    for (const std::string &element : clientDelayedOutput_)
    {
        std::cout << element;
    }

    clientDelayedOutput_.clear ();
}

void Context::RegisterMessages ()
{
    // TODO: Handle register failures in release mode.
    Hotline::ResultCode result;
    result = socketClient_.CoreContext ().RegisterFactory (
        static_cast <Hotline::MessageTypeId> (Messaging::Message::VOID_OPERATION_RESULT_RESPONSE),
        [this] () -> Hotline::MessageParser
        {
            return Messaging::VoidOperationResultResponse::CreateParserWithCallback (
                [this] (const Messaging::VoidOperationResultResponse &message, Hotline::SocketSession *session)
                {
                    AddDelayedOutput (
                        "Received response to query " + std::to_string (message.queryId_) +
                        ". Operation result code is " + Messaging::GetOperationResultName (message.result_) +
                        ".\n");
                });
        });

    assert (result == Hotline::ResultCode::OK);
    result = socketClient_.CoreContext ().RegisterFactory (
        static_cast <Hotline::MessageTypeId> (Messaging::Message::CREATE_OPERATION_RESULT_RESPONSE),
        [this] () -> Hotline::MessageParser
        {
            return Messaging::CreateOperationResultResponse::CreateParserWithCallback (
                [this] (const Messaging::CreateOperationResultResponse &message, Hotline::SocketSession *session)
                {
                    AddDelayedOutput (
                        "Received response to query " + std::to_string (message.queryId_) +
                        ". Created item id is " + std::to_string (message.resourceId_) + ".\n");
                });
        });

    assert (result == Hotline::ResultCode::OK);

    result = socketClient_.CoreContext ().RegisterFactory (
        static_cast <Hotline::MessageTypeId> (Messaging::Message::RESOURCE_IDS_RESPONSE),
        [this] () -> Hotline::MessageParser
        {
            return Messaging::IdsResponse::CreateParserWithCallback (
                [this] (const Messaging::IdsResponse &message, Hotline::SocketSession *session)
                {
                    std::string output = "Received response to query " + std::to_string (message.queryId_) +
                                         ". Result resource ids are:\n";

                    for (Messaging::ResourceId id : message.ids_)
                    {
                        output += " - " + std::to_string (id) + "\n";
                    }

                    AddDelayedOutput (output);
                });
        });

    assert (result == Hotline::ResultCode::OK);
    result = socketClient_.CoreContext ().RegisterFactory (
        static_cast <Hotline::MessageTypeId> (Messaging::Message::GET_TABLE_NAME_RESPONSE),
        [this] () -> Hotline::MessageParser
        {
            return Messaging::GetTableNameResponse::CreateParserWithCallback (
                [this] (const Messaging::GetTableNameResponse &message, Hotline::SocketSession *session)
                {
                    AddDelayedOutput (
                        "Received response to query " + std::to_string (message.queryId_) +
                        ". Table name is " + message.tableName_ + ".\n");
                });
        });

    assert (result == Hotline::ResultCode::OK);
    result = socketClient_.CoreContext ().RegisterFactory (
        static_cast <Hotline::MessageTypeId> (Messaging::Message::GET_COLUMN_INFO_RESPONSE),
        [this] () -> Hotline::MessageParser
        {
            return Messaging::ColumnInfoResponse::CreateParserWithCallback (
                [this] (const Messaging::ColumnInfoResponse &message, Hotline::SocketSession *session)
                {
                    AddDelayedOutput (
                        "Received response to query " + std::to_string (message.queryId_) +
                        ". Column name is " + message.name_ + ", data type is " +
                        Richard::GetDataTypeName (message.dataType_) + "\n");
                });
        });

    assert (result == Hotline::ResultCode::OK);
    result = socketClient_.CoreContext ().RegisterFactory (
        static_cast <Hotline::MessageTypeId> (Messaging::Message::GET_INDEX_INFO_RESPONSE),
        [this] () -> Hotline::MessageParser
        {
            return Messaging::IndexInfoResponse::CreateParserWithCallback (
                [this] (const Messaging::IndexInfoResponse &message, Hotline::SocketSession *session)
                {
                    std::string output = "Received response to query " + std::to_string (message.queryId_) +
                                         ". Index name is " + message.name_ + ", base columns are:\n";

                    for (Messaging::ResourceId id : message.columns_)
                    {
                        output += " - " + std::to_string (id) + "\n";
                    }

                    AddDelayedOutput (output);
                });
        });

    assert (result == Hotline::ResultCode::OK);
    result = socketClient_.CoreContext ().RegisterFactory (
        static_cast <Hotline::MessageTypeId> (Messaging::Message::CURSOR_GET_RESPONSE),
        [this] () -> Hotline::MessageParser
        {
            return Messaging::CursorGetResponse::CreateParserWithCallback (
                [this] (const Messaging::CursorGetResponse &message, Hotline::SocketSession *session)
                {
                    std::string output = "Received response to query " + std::to_string (message.queryId_) +
                                         ". Value type is " + Richard::GetDataTypeName (message.value_.GetType ()) +
                                         ", value is \"";

                    switch (message.value_.GetType ())
                    {
                        case Richard::DataType::INT8:
                            output += std::to_string (*reinterpret_cast <const int8_t *> (
                                message.value_.GetDataStartPointer ()));
                            break;

                        case Richard::DataType::INT16:
                            output += std::to_string (*reinterpret_cast <const int16_t *> (
                                message.value_.GetDataStartPointer ()));
                            break;

                        case Richard::DataType::INT32:
                            output += std::to_string (*reinterpret_cast <const int32_t *> (
                                message.value_.GetDataStartPointer ()));
                            break;

                        case Richard::DataType::INT64:
                            output += std::to_string (*reinterpret_cast <const int64_t *> (
                                message.value_.GetDataStartPointer ()));
                            break;

                        case Richard::DataType::SHORT_STRING:
                        case Richard::DataType::STRING:
                        case Richard::DataType::LONG_STRING:
                        case Richard::DataType::HUGE_STRING:
                        case Richard::DataType::BLOB_16KB:
                        {
                            const char *valuePointer = reinterpret_cast <const char *> (
                                message.value_.GetDataStartPointer ());
                            const char *end = valuePointer + Richard::GetDataTypeSize (message.value_.GetType ());

                            while (valuePointer != end && *valuePointer)
                            {
                                output += *valuePointer;
                                ++valuePointer;
                            }

                            break;
                        }
                    }

                    output += "\".\n";
                    AddDelayedOutput (output);
                });
        });

    assert (result == Hotline::ResultCode::OK);
}

void Context::AddDelayedOutput (const std::string &element)
{
    std::unique_lock <std::mutex> lock (clientDelayedOutputGuard_);
    clientDelayedOutput_.emplace_back (element);
}
}
