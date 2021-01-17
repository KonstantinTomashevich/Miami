// Not only we don't need GDI, but it also has ERROR macro that breaks Evan's LogLevel.
#define NOGDI

#include <cassert>

#include <boost/asio/connect.hpp>

#include <Miami/Evan/Logger.hpp>

#include <Miami/Hotline/SocketClient.hpp>
#include <Miami/Hotline/SocketSession.hpp>

namespace Miami::Hotline
{
SocketClient::SocketClient (Disco::Context *multithreadingContext)
    : multithreadingContext_ (multithreadingContext),
      socketContext_ ()
{
    assert (multithreadingContext_);
}

ResultCode SocketClient::Start (const std::string &host, const std::string &service)
{
    if (!multithreadingContext_)
    {
        assert (false);
        return ResultCode::INVARIANTS_VIOLATED;
    }

    boost::asio::ip::tcp::resolver resolver (socketContext_.asioContext_);
    boost::system::error_code error;

    boost::asio::ip::tcp::resolver::results_type endpoints =
        resolver.resolve (host.c_str (), service.c_str (), error);

    if (error)
    {
        Evan::Logger::Get ().Log (
            Evan::LogLevel::ERROR,
            "Unable to receive available endpoints due to io error: " + error.message () + ".");
        return ResultCode::SOCKET_IO_ERROR;
    }

    if (endpoints.empty ())
    {
        return ResultCode::ENDPOINTS_NOT_FOUND;
    }

    auto *socketSession = new SocketSession (multithreadingContext_, &socketContext_);
    boost::asio::connect (SocketContext::RetrieveSessionSocket (socketSession), endpoints, error);

    if (error)
    {
        Evan::Logger::Get ().Log (
            Evan::LogLevel::ERROR,
            "Unable to connect to endpoint due to io error: " + error.message () + ".");
        return ResultCode::SOCKET_IO_ERROR;
    }

    ResultCode result = socketContext_.AddSession (socketSession);
    if (result != ResultCode::OK)
    {
        delete socketSession;
    }

    return result;
}

SocketContext &SocketClient::CoreContext ()
{
    return socketContext_;
}

const SocketContext &SocketClient::CoreContext () const
{
    return socketContext_;
}
}
