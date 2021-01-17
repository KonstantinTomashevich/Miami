// Not only we don't need GDI, but it also has ERROR macro that breaks Evan's LogLevel.
#define NOGDI

#include <atomic>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <csignal>

#include <App/Miami/Client/Context.hpp>
#include <App/Miami/Messaging/Message.hpp>

#include <Miami/Evan/GlobalLogger.hpp>
#include <Miami/Evan/Logger.hpp>
#include <memory>

enum class ExitCode
{
    OK = 0,
    INCORRECT_ARGUMENTS,
    UNABLE_TO_SETUP_LOGGING,
    CLIENT_CONNECT_ERROR,
};

#define Exit(Code) return static_cast<int>(Code)

struct CommandLineArguments
{
    std::string host_;
    std::string service_;
    std::string logFileName_;
};

std::atomic <char> running = true;

void ProcessSignal (int signal);

bool ParseCommandLineArguments (int argc, char **argv, CommandLineArguments &output);

bool SetupLogging (const CommandLineArguments &arguments);

void PrintMessageIndices ();

bool InputAndSendRequest (uint64_t nextQueryId, Miami::Hotline::SocketSession *session);

int main (int argc, char **argv)
{
    signal (SIGINT, ProcessSignal);
    signal (SIGTERM, ProcessSignal);

    CommandLineArguments arguments;
    if (!ParseCommandLineArguments (argc, argv, arguments))
    {
        Exit (ExitCode::INCORRECT_ARGUMENTS);
    }

    if (!SetupLogging (arguments))
    {
        Exit (ExitCode::UNABLE_TO_SETUP_LOGGING);
    }

    auto clientContext = std::make_unique <Miami::App::Client::Context> ();
    Miami::App::Client::ResultCode result = clientContext->Connect (arguments.host_, arguments.service_);

    if (result != Miami::App::Client::ResultCode::OK)
    {
        printf ("Client connect step resulted in error with code %d!", result);
        Exit (ExitCode::CLIENT_CONNECT_ERROR);
    }

    PrintMessageIndices ();
    std::thread socketIOThread (
        [&clientContext] ()
        {
            while (running)
            {
                Miami::Hotline::ResultCode result = clientContext->Client ().CoreContext ().DoStep ();
                if (result != Miami::Hotline::ResultCode::OK)
                {
                    Miami::Evan::Logger::Get ().Log (
                        Miami::Evan::LogLevel::ERROR, "Caught Hotline error during socket client step " +
                                                      std::to_string (static_cast <uint32_t> (result)) + "!");
                    running = false;
                }

                bool anySessions = clientContext->Client ().CoreContext ().HasAnySession ();
                std::atomic_fetch_and (&running, anySessions);
            }
        });

    uint64_t nextQueryId = 0;
    while (running)
    {
        std::cout << "Press q to quit, c to input command and m to view pending messages." << std::endl;
        char selection;
        std::cin >> selection;

        switch (selection)
        {
            case 'q':
                running = false;
                break;

            case 'c':
                if (InputAndSendRequest (nextQueryId, clientContext->Client ().GetSession()))
                {
                    std::cout << "This query will have id " << nextQueryId << "." << std::endl;
                    ++nextQueryId;
                }

                break;

            case 'm':
                clientContext->PrintAllDelayedOutput ();
                break;
        }
    }

    socketIOThread.join ();
    Exit (ExitCode::OK);
}

void ProcessSignal (int signal)
{
    if (signal == SIGINT || signal == SIGTERM)
    {
        running = false;
    }
}

bool ParseCommandLineArguments (int argc, char **argv, CommandLineArguments &output)
{
    if (argc != 4)
    {
        printf ("Expected command line: <executable> <host> <service> <log_file_name>");
        return false;
    }
    else
    {
        output.host_ = argv[1];
        output.service_ = argv[2];
        output.logFileName_ = argv[3];
        return true;
    }
}

bool SetupLogging (const CommandLineArguments &arguments)
{
    Miami::Evan::GlobalLogger::Instance ().AddOutput (
        std::make_shared <std::ostream> (std::cout.rdbuf ()), Miami::Evan::LogLevel::VERBOSE);

    auto fileSharedStream = std::make_shared <std::ofstream> (arguments.logFileName_);
    if (*fileSharedStream)
    {
        Miami::Evan::GlobalLogger::Instance ().AddOutput (fileSharedStream, Miami::Evan::LogLevel::VERBOSE);
        return true;
    }
    else
    {
        printf ("Unable to setup log output to file %s!", arguments.logFileName_.c_str ());
        return false;
    }
}

void PrintMessageIndices ()
{
    std::cout << "Supported messages:" << std::endl;
    auto message = static_cast <Miami::App::Messaging::Message> (0u);

    while (message <= Miami::App::Messaging::Message::REMOVE_TABLE_REQUEST)
    {
        std::cout << "  " << static_cast<uint64_t> (message) << ". " <<
                  Miami::App::Messaging::GetMessageName (message) << std::endl;
        message = static_cast <Miami::App::Messaging::Message> (static_cast<uint64_t> (message) + 1u);
    }
}

bool InputAndSendRequest (uint64_t nextQueryId, Miami::Hotline::SocketSession *session)
{
    // TODO: Temporary method implementation, rewrite later.
    if (!session)
    {
        assert (session);
        return false;
    }

    Miami::App::Messaging::Message messageType;
    {
        uint64_t rawIndex = ~0;
        std::cout << "Input message type index: ";
        std::cin >> rawIndex;
        messageType = static_cast <Miami::App::Messaging::Message> (rawIndex);
    }

    auto inputDataType = [] ()
    {
        uint64_t rawIndex = ~0;
        std::cin >> rawIndex;
        return static_cast <Miami::Richard::DataType> (rawIndex);
    };

    // TODO: Temporary adhok implementation.
    auto inputTableUpdateValue = [inputDataType] ()
    {
        Miami::App::Messaging::ResourceId columnId;
        std::cout << "Input column id: ";
        std::cin >> columnId;

        std::cout << "Input data type index: ";
        Miami::Richard::DataType dataType = inputDataType ();
        Miami::Richard::AnyDataContainer container (dataType);

        std::cout << "Input value: ";
        switch (dataType)
        {
            case Miami::Richard::DataType::INT8:
            {
                int8_t value;
                std::cin >> value;
                *static_cast<int8_t *> (container.GetDataStartPointer ()) = value;
            }
                break;

            case Miami::Richard::DataType::INT16:
            {
                int16_t value;
                std::cin >> value;
                *static_cast<int16_t *> (container.GetDataStartPointer ()) = value;
            }
                break;

            case Miami::Richard::DataType::INT32:
            {
                int32_t value;
                std::cin >> value;
                *static_cast<int32_t *> (container.GetDataStartPointer ()) = value;
            }
                break;

            case Miami::Richard::DataType::INT64:
            {
                int64_t value;
                std::cin >> value;
                *static_cast<int64_t *> (container.GetDataStartPointer ()) = value;
            }
                break;

            case Miami::Richard::DataType::SHORT_STRING:
            case Miami::Richard::DataType::STRING:
            case Miami::Richard::DataType::LONG_STRING:
            case Miami::Richard::DataType::HUGE_STRING:
            case Miami::Richard::DataType::BLOB_16KB:
            {
                std::string value;
                std::cin >> value;
                memcpy (&value[0], container.GetDataStartPointer (),
                        std::min (static_cast<uint32_t>(value.size ()), Miami::Richard::GetDataTypeSize (dataType)));
            }
                break;
        }

        return std::make_pair (columnId, Miami::Richard::AnyDataContainer (std::move (container)));
    };

    switch (messageType)
    {
        case Miami::App::Messaging::Message::GET_TABLE_READ_ACCESS_REQUEST:
        case Miami::App::Messaging::Message::GET_TABLE_WRITE_ACCESS_REQUEST:
        case Miami::App::Messaging::Message::CLOSE_TABLE_READ_ACCESS_REQUEST:
        case Miami::App::Messaging::Message::CLOSE_TABLE_WRITE_ACCESS_REQUEST:
        case Miami::App::Messaging::Message::GET_TABLE_NAME_REQUEST:
        case Miami::App::Messaging::Message::GET_COLUMNS_IDS_REQUEST:
        case Miami::App::Messaging::Message::GET_INDICES_IDS_REQUEST:
        {
            Miami::App::Messaging::TableOperationRequest request {};

            request.queryId_ = nextQueryId;
            std::cout << "Input table id: ";
            std::cin >> request.tableId_;

            request.Write (messageType, session);
            return true;
        }

        case Miami::App::Messaging::Message::CREATE_READ_CURSOR_REQUEST:
        case Miami::App::Messaging::Message::GET_COLUMN_INFO_REQUEST:
        case Miami::App::Messaging::Message::GET_INDEX_INFO_REQUEST:
        case Miami::App::Messaging::Message::CREATE_EDIT_CURSOR_REQUEST:
        case Miami::App::Messaging::Message::REMOVE_COLUMN_REQUEST:
        case Miami::App::Messaging::Message::REMOVE_INDEX_REQUEST:
        {
            Miami::App::Messaging::TablePartOperationRequest request {};

            request.queryId_ = nextQueryId;
            std::cout << "Input table id: ";
            std::cin >> request.tableId_;

            std::cout << "Input target part id: ";
            std::cin >> request.partId_;

            request.Write (messageType, session);
            return true;
        }

        case Miami::App::Messaging::Message::SET_TABLE_NAME_REQUEST:
        {
            Miami::App::Messaging::SetTableNameRequest request {};

            request.queryId_ = nextQueryId;
            std::cout << "Input table id: ";
            std::cin >> request.tableId_;

            std::cout << "Input new name: ";
            std::cin >> request.newName_;

            request.Write (messageType, session);
            return true;
        }

        case Miami::App::Messaging::Message::ADD_COLUMN_REQUEST:
        {
            Miami::App::Messaging::AddColumnRequest request {};

            request.queryId_ = nextQueryId;
            std::cout << "Input table id: ";
            std::cin >> request.tableId_;

            std::cout << "Input column name: ";
            std::cin >> request.name_;

            std::cout << "Input column data type index: ";
            request.dataType_ = inputDataType ();

            request.Write (messageType, session);
            return true;
        }
        case Miami::App::Messaging::Message::ADD_INDEX_REQUEST:
        {
            Miami::App::Messaging::AddIndexRequest request {};

            request.queryId_ = nextQueryId;
            std::cout << "Input table id: ";
            std::cin >> request.tableId_;

            std::cout << "Input index name: ";
            std::cin >> request.name_;

            uint64_t baseColumnsCount;
            std::cout << "Input base columns count: ";
            std::cin >> baseColumnsCount;

            while (baseColumnsCount--)
            {
                Miami::App::Messaging::ResourceId columnId;
                std::cout << "Input column id: ";
                std::cin >> columnId;
                request.columns_.emplace_back (columnId);
            }

            request.Write (messageType, session);
            return true;
        }
        case Miami::App::Messaging::Message::ADD_ROW_REQUEST:
        {
            Miami::App::Messaging::AddRowRequest request {};

            request.queryId_ = nextQueryId;
            std::cout << "Input table id: ";
            std::cin >> request.tableId_;

            uint64_t valuesCount;
            std::cout << "Input values count: ";
            std::cin >> valuesCount;

            while (valuesCount--)
            {
                request.values_.emplace_back (inputTableUpdateValue ());
            }

            request.Write (messageType, session);
            return true;
        }
        case Miami::App::Messaging::Message::CURSOR_ADVANCE_REQUEST:
        {
            Miami::App::Messaging::CursorAdvanceRequest request {};

            request.queryId_ = nextQueryId;
            std::cout << "Input cursor id: ";
            std::cin >> request.cursorId_;

            std::cout << "Input step: ";
            std::cin >> request.step_;

            request.Write (messageType, session);
            return true;
        }
        case Miami::App::Messaging::Message::CURSOR_GET_REQUEST:
        {
            Miami::App::Messaging::CursorGetRequest request {};

            request.queryId_ = nextQueryId;
            std::cout << "Input cursor id: ";
            std::cin >> request.cursorId_;

            std::cout << "Input column id: ";
            std::cin >> request.columnId_;

            request.Write (messageType, session);
            return true;
        }
        case Miami::App::Messaging::Message::CURSOR_UPDATE_REQUEST:
        {
            Miami::App::Messaging::CursorUpdateRequest request {};

            request.queryId_ = nextQueryId;
            std::cout << "Input cursor id: ";
            std::cin >> request.cursorId_;

            uint64_t valuesCount;
            std::cout << "Input values count: ";
            std::cin >> valuesCount;

            while (valuesCount--)
            {
                request.values_.emplace_back (inputTableUpdateValue ());
            }

            request.Write (messageType, session);
            return true;
        }
        case Miami::App::Messaging::Message::CURSOR_DELETE_REQUEST:
        {
            Miami::App::Messaging::CursorVoidActionRequest request {};

            request.queryId_ = nextQueryId;
            std::cout << "Input cursor id: ";
            std::cin >> request.cursorId_;

            request.Write (messageType, session);
            return true;
        }
        case Miami::App::Messaging::Message::CLOSE_CURSOR_REQUEST:
        {
            Miami::App::Messaging::CursorVoidActionRequest request {};

            request.queryId_ = nextQueryId;
            std::cout << "Input cursor id: ";
            std::cin >> request.cursorId_;

            request.Write (messageType, session);
            return true;
        }
        case Miami::App::Messaging::Message::GET_CONDUIT_READ_ACCESS_REQUEST:
        {
            Miami::App::Messaging::ConduitVoidActionRequest request {};
            request.queryId_ = nextQueryId;
            request.Write (messageType, session);
            return true;
        }
        case Miami::App::Messaging::Message::GET_CONDUIT_WRITE_ACCESS_REQUEST:
        {
            Miami::App::Messaging::ConduitVoidActionRequest request {};
            request.queryId_ = nextQueryId;
            request.Write (messageType, session);
            return true;
        }
        case Miami::App::Messaging::Message::CLOSE_CONDUIT_READ_ACCESS_REQUEST:
        {
            Miami::App::Messaging::ConduitVoidActionRequest request {};
            request.queryId_ = nextQueryId;
            request.Write (messageType, session);
            return true;
        }
        case Miami::App::Messaging::Message::CLOSE_CONDUIT_WRITE_ACCESS_REQUEST:
        {
            Miami::App::Messaging::ConduitVoidActionRequest request {};
            request.queryId_ = nextQueryId;
            request.Write (messageType, session);
            return true;
        }
        case Miami::App::Messaging::Message::GET_TABLE_IDS_REQUEST:
        {
            Miami::App::Messaging::ConduitVoidActionRequest request {};
            request.queryId_ = nextQueryId;
            request.Write (messageType, session);
            return true;
        }
        case Miami::App::Messaging::Message::ADD_TABLE_REQUEST:
        {
            Miami::App::Messaging::AddTableRequest request {};
            request.queryId_ = nextQueryId;

            std::cout << "Input table name: ";
            std::cin >> request.tableName_;

            request.Write (messageType, session);
            return true;
        }
        case Miami::App::Messaging::Message::REMOVE_TABLE_REQUEST:
        {
            Miami::App::Messaging::TableOperationRequest request {};
            request.queryId_ = nextQueryId;

            std::cout << "Input table id: ";
            std::cin >> request.tableId_;

            request.Write (messageType, session);
            return true;
        }

        default:
            std::cout << "Given message type is not a request type!" << std::endl;
            return false;
    }
}