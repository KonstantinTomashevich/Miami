#pragma once

#include <vector>
#include <memory>
#include <unordered_map>

#include <boost/asio/io_context.hpp>

#include <Miami/Annotations.hpp>

#include <Miami/Hotline/Message.hpp>
#include <Miami/Hotline/ResultCode.hpp>
#include <Miami/Hotline/SocketSession.hpp>

namespace Miami::Hotline
{
class SocketContext final
{
public:
    static constexpr uint32_t CALLBACKS_PER_CLEANUP_CYCLE = 1000;

    SocketContext ();

    free_call bool HasAnySession () const;

    free_call ResultCode DoStep ();

    free_call ResultCode RegisterFactory (MessageTypeId messageType, MessageParserFactory &&factory);

private:
    free_call MessageParser ConstructParser (MessageTypeId messageType) const;

    free_call static boost::asio::ip::tcp::socket &RetrieveSessionSocket (SocketSession *session);

    free_call ResultCode AddSession (moved_in std::unique_ptr <SocketSession> &session);

    std::vector <std::unique_ptr <SocketSession>> sessions_;

    // TODO: Use flat hash map.
    std::unordered_map <MessageTypeId, MessageParserFactory> parserFactories_;
    boost::asio::io_context asioContext_;
    std::size_t callbacksExecuted_;

    friend class SocketSession;

    friend class SocketServer;

    friend class SocketClient;
};
}
