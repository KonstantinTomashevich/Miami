#pragma once

#include <cstdint>

#include <boost/asio/ip/tcp.hpp>

#include <Miami/Annotations.hpp>

#include <Miami/Disco/Context.hpp>

#include <Miami/Hotline/SocketContext.hpp>

namespace Miami::Hotline
{
class SocketServer final
{
public:
    explicit SocketServer (Disco::Context *multithreadingContext);

    free_call ResultCode Start (uint16_t port, bool useIPv6);

    SocketContext &CoreContext ();

    const SocketContext &CoreContext () const;

private:
    free_call void ContinueAccepting ();

    Disco::Context *multithreadingContext_;
    SocketContext socketContext_;
    boost::asio::ip::tcp::acceptor acceptor_;
};
}
