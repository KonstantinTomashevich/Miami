#pragma once

#include <string>

#include <Miami/Annotations.hpp>

#include <Miami/Disco/Context.hpp>

#include <Miami/Hotline/SocketContext.hpp>

namespace Miami::Hotline
{
class SocketClient final
{
public:
    explicit SocketClient (Disco::Context *multithreadingContext);

    free_call ResultCode Start (const std::string &host, const std::string &service);

    free_call SocketSession *GetSession ();

    SocketContext &CoreContext ();

    const SocketContext &CoreContext () const;

private:
    Disco::Context *multithreadingContext_;
    SocketContext socketContext_;
};
}