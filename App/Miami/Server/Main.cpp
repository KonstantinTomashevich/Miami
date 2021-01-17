// Not only we don't need GDI, but it also has ERROR macro that breaks Evan's LogLevel.
#define NOGDI

#include <cstdio>
#include <iostream>
#include <fstream>
#include <csignal>

#include <App/Miami/Server/Context.hpp>

#include <Miami/Evan/GlobalLogger.hpp>
#include <Miami/Evan/Logger.hpp>
#include <memory>

enum class ExitCode
{
    OK = 0,
    INCORRECT_ARGUMENTS,
    UNABLE_TO_SETUP_LOGGING,
    SERVER_ERROR,
};

#define Exit(Code) return static_cast<int>(Code)

struct CommandLineArguments
{
    uint16_t port_;
    uint32_t workerThreads_;
    std::string logFileName_;
};

static std::unique_ptr<Miami::App::Server::Context> serverContext = nullptr;

void ProcessSignal (int signal);

bool ParseCommandLineArguments (int argc, char **argv, CommandLineArguments &output);

bool SetupLogging (const CommandLineArguments &arguments);

int main (int argc, char **argv)
{
    signal(SIGINT, ProcessSignal);
    signal(SIGTERM, ProcessSignal);

    CommandLineArguments arguments;
    if (!ParseCommandLineArguments (argc, argv, arguments))
    {
        Exit (ExitCode::INCORRECT_ARGUMENTS);
    }

    if (!SetupLogging (arguments))
    {
        Exit (ExitCode::UNABLE_TO_SETUP_LOGGING);
    }

    serverContext = std::make_unique<Miami::App::Server::Context>(arguments.workerThreads_);
    Miami::App::Server::ResultCode result = serverContext->Execute(arguments.port_);
    serverContext.reset();

    if (result == Miami::App::Server::ResultCode::OK)
    {
        Exit (ExitCode::OK);
    }
    else
    {
        printf ("Server execution resulted in error with code %d!", result);
        Exit (ExitCode::SERVER_ERROR);
    }
}

void ProcessSignal (int signal)
{
    if ((signal == SIGINT || signal == SIGTERM) && serverContext)
    {
        serverContext->RequestAbort();
    }
}

bool ParseCommandLineArguments (int argc, char **argv, CommandLineArguments &output)
{
    if (argc != 4)
    {
        printf ("Expected command line: <executable> <server_port> <worker_threads> <log_file_name>");
        return false;
    }
    else
    {
        int port = atoi (argv[1]);
        if (port < 1 || port > std::numeric_limits <uint16_t>::max ())
        {
            printf ("Port must be positive non-zero 16-bit integer!");
            return false;
        }

        int workerThreads = atoi (argv[2]);
        if (workerThreads < 1 || workerThreads > std::numeric_limits <uint32_t>::max ())
        {
            printf ("Worker threads count must be positive non-zero 32-bit integer!");
            return false;
        }

        output.port_ = static_cast<uint16_t>(port);
        output.workerThreads_ = static_cast<uint32_t>(workerThreads);
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