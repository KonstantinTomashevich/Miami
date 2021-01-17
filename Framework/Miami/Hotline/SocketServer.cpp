// Not only we don't need GDI, but it also has ERROR macro that breaks Evan's LogLevel.
#define NOGDI

#include <cassert>

#include <Miami/Evan/Logger.hpp>

#include <Miami/Hotline/SocketServer.hpp>
#include <Miami/Hotline/SocketSession.hpp>

namespace Miami::Hotline
{
SocketServer::SocketServer (Disco::Context *multithreadingContext)
    : multithreadingContext_ (multithreadingContext),
      socketContext_ (),
      acceptor_ (socketContext_.asioContext_)
{
    assert (multithreadingContext_);
}

ResultCode SocketServer::Start (uint16_t port, bool useIPv6)
{
    boost::asio::ip::tcp::endpoint endpoint (
        useIPv6 ? boost::asio::ip::tcp::v6 () : boost::asio::ip::tcp::v4 (), port);

    boost::system::error_code error;
    acceptor_.open (endpoint.protocol (), error);

    if (error)
    {
        Evan::Logger::Get ().Log (
            Evan::LogLevel::ERROR,
            "Unable to open socket acceptor due to io error: " + error.message () + ".");
        return ResultCode::SOCKET_IO_ERROR;
    }

    acceptor_.bind (endpoint, error);
    if (error)
    {
        Evan::Logger::Get ().Log (
            Evan::LogLevel::ERROR,
            "Unable to bind endpoint to acceptor due to io error: " + error.message () + ".");
        return ResultCode::SOCKET_IO_ERROR;
    }

    acceptor_.listen(boost::asio::socket_base::max_listen_connections, error);
    if (error)
    {
        Evan::Logger::Get ().Log (
            Evan::LogLevel::ERROR,
            "Unable to set acceptor to list mode due to io error: " + error.message () + ".");
        return ResultCode::SOCKET_IO_ERROR;
    }

    ContinueAccepting ();
    return ResultCode::OK;
}

SocketContext &SocketServer::CoreContext ()
{
    return socketContext_;
}

const SocketContext &SocketServer::CoreContext () const
{
    return socketContext_;
}

void SocketServer::ContinueAccepting ()
{
    assert (acceptor_.is_open ());
    auto *newSocketSession = new SocketSession (multithreadingContext_, &socketContext_);

    acceptor_.async_accept (
        SocketContext::RetrieveSessionSocket (newSocketSession),
        [this, newSocketSession] (const boost::system::error_code &error) mutable
        {
            if (error)
            {
                Evan::Logger::Get ().Log (
                    Evan::LogLevel::ERROR,
                    "Caught socket io error in accept callback: " + error.message () +
                    ". Shutting down server!");
                delete newSocketSession;
            }
            else
            {
                ResultCode registrationResult = socketContext_.AddSession (newSocketSession);
                if (registrationResult == ResultCode::OK)
                {
                    ContinueAccepting ();
                }
                else
                {
                    Evan::Logger::Get ().Log (
                        Evan::LogLevel::ERROR,
                        "Caught not-ok result code " +
                        std::to_string (static_cast<uint64_t>(registrationResult)) +
                        " during session registration process. Shutting down server!");
                    delete newSocketSession;
                }
            }
        });
}
}
