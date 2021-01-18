#pragma once

#include <mutex>
#include <vector>
#include <string>

#include <Miami/Disco/Context.hpp>

#include <Miami/Hotline/SocketClient.hpp>

namespace Miami::App::Client
{
enum class ResultCode
{
    OK = 0,
    UNABLE_TO_CONNECT,
};

class Context final
{
public:
    Context ();

    free_call ResultCode Connect (const std::string &host, const std::string &service);

    free_call Hotline::SocketClient &Client ();

    // TODO: Adhok method.
    void PrintAllDelayedOutput ();

private:
    void RegisterMessages ();

    // TODO: Adhok method.
    void AddDelayedOutput (const std::string &element);

    Disco::Context multithreadingContext_;
    Hotline::SocketClient socketClient_;

    std::vector <std::string> clientDelayedOutput_;
    std::mutex clientDelayedOutputGuard_;
};
}
