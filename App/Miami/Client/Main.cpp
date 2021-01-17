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
                std::atomic_fetch_and(&running, anySessions);
            }
        });

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
            {
                // TODO: Implement.
                Miami::App::Messaging::ConduitVoidActionRequest request {5};
                // TODO: A bit adhok, write is "safe" only because there is current write implementation with mutex.
                request.Write (Miami::App::Messaging::Message::GET_TABLE_IDS_REQUEST,
                               clientContext->Client ().GetSession ());
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