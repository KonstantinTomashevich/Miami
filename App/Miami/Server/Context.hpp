#pragma once

#include <atomic>

#include <Miami/Annotations.hpp>

#include <Miami/Disco/Context.hpp>

#include <Miami/Richard/Conduit.hpp>

#include <Miami/Hotline/SocketServer.hpp>

namespace Miami::App::Server
{
enum class ResultCode
{
    OK = 0,
    UNABLE_TO_START_SOCKET_SERVER,
};

class Context final
{
public:
    explicit Context (uint32_t workerThreads);

    free_call ResultCode Execute (uint16_t port);

    void RequestAbort ();

private:
    void RegisterMessages ();

    std::atomic <bool> alive_;
    Disco::Context multithreadingContext_;
    Richard::Conduit databaseConduit_;
    Hotline::SocketServer socketServer_;
};
}
