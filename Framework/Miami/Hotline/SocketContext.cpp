// Not only we don't need GDI, but it also has ERROR macro that breaks Evan's LogLevel.
#define NOGDI

#include <cassert>

#include <Miami/Evan/Logger.hpp>

#include <Miami/Hotline/SocketContext.hpp>

namespace Miami::Hotline
{
SocketContext::SocketContext ()
    : sessions_ (),
      parserFactories_ (),
      asioContext_ (),
      callbacksExecuted_ (0)
{
}

bool SocketContext::HasAnySession () const
{
    return !sessions_.empty ();
}

ResultCode SocketContext::DoStep ()
{
    boost::system::error_code error;
    std::size_t callbacksExecuted = asioContext_.poll (error);

    if (callbacksExecuted_ >= CALLBACKS_PER_CLEANUP_CYCLE)
    {
        callbacksExecuted_ = 0;
        auto iterator = sessions_.begin ();

        while (iterator != sessions_.end ())
        {
            if ((**iterator).valid_)
            {
                ++iterator;
            }
            else
            {
                sessions_.erase (iterator);
            }
        }
    }
    else
    {
        callbacksExecuted_ += callbacksExecuted;
    }

    if (error)
    {
        Evan::Logger::Get ().Log (
            Evan::LogLevel::ERROR,
            "Unable to poll socket io events due to error: " + error.message () + ".");
        return ResultCode::SOCKET_IO_ERROR;
    }
    else
    {
        return ResultCode::OK;
    }
}

ResultCode SocketContext::RegisterFactory (MessageTypeId messageType, MessageParserFactory &&factory)
{
    if (!factory)
    {
        assert (false);
        return ResultCode::FACTORY_CAN_NOT_BE_NULL;
    }

    auto iterator = parserFactories_.find (messageType);
    if (iterator == parserFactories_.end ())
    {
        return ResultCode::FACTORY_FOR_MESSAGE_TYPE_IS_ALREADY_REGISTERED;
    }
    else
    {
        auto result = parserFactories_.emplace (messageType, factory);
        if (result.second)
        {
            return ResultCode::OK;
        }
        else
        {
            Evan::Logger::Get ().Log (
                Evan::LogLevel::ERROR,
                "Unable to register message parser factory, because emplace operation failed!");

            assert (false);
            return ResultCode::INVARIANTS_VIOLATED;
        }
    }
}

MessageParser SocketContext::ConstructParser (MessageTypeId messageType) const
{
    auto iterator = parserFactories_.find (messageType);
    if (iterator == parserFactories_.end ())
    {
        return nullptr;
    }
    else
    {
        return iterator->second ();
    }
}

boost::asio::ip::tcp::socket &SocketContext::RetrieveSessionSocket (SocketSession *session)
{
    assert (session);
    return session->socket_;
}

ResultCode SocketContext::AddSession (std::unique_ptr <SocketSession> &session)
{
    assert (session);
    if (session)
    {
        ResultCode startResult = session->Start ();
        if (startResult == ResultCode::OK)
        {
            // New session saved only if it's able to start without errors.
            sessions_.emplace_back (std::move (session));
        }

        return startResult;
    }
    else
    {
        Evan::Logger::Get ().Log (
            Evan::LogLevel::ERROR,
            "Caught attempt to add null session to context!");

        // Technically, it's OK to ignore nullptr sessions.
        return ResultCode::OK;
    }
}
}
