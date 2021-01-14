// Not only we don't need GDI, but it also has ERROR macro that breaks Evan's LogLevel.
#define NOGDI

#include <cassert>

#include <boost/asio/write.hpp>
#include <boost/asio/read.hpp>

#include <Miami/Evan/Logger.hpp>

#include <Miami/Hotline/SocketSession.hpp>
#include <Miami/Hotline/SocketContext.hpp>

namespace Miami::Hotline
{
SocketSession::SocketSession (Disco::Context *multithreadingContext, SocketContext *socketContext)
    : outputBufferGuard_ (),
      outputBuffer_ (),

      session_ (multithreadingContext),
      socket_ (socketContext->asioContext_),
      context_ (socketContext),

      currentMessageParser_ (),
      inputBuffer_ (),
      valid_ (false)
{
    assert (context_);
    if (!context_)
    {
        valid_ = false;
    }
}

ResultCode SocketSession::Write (const MemoryRegion &region)
{
    if (!valid_)
    {
        return ResultCode::INVALID_SOCKET_SESSION;
    }

    std::unique_lock <std::mutex> lock (outputBufferGuard_);
    return WriteInternal (region);
}

ResultCode SocketSession::Write (const std::vector <MemoryRegion> &regions)
{
    if (!valid_)
    {
        return ResultCode::INVALID_SOCKET_SESSION;
    }

    std::unique_lock <std::mutex> lock (outputBufferGuard_);
    for (const MemoryRegion &region : regions)
    {
        ResultCode result = WriteInternal (region);
        if (result != ResultCode::OK)
        {
            return result;
        }
    }

    return ResultCode::OK;
}

Session &SocketSession::Data ()
{
    return session_;
}

ResultCode SocketSession::Start ()
{
    valid_ = socket_.is_open ();
    if (!valid_)
    {
        Evan::Logger::Get ().Log (Evan::LogLevel::ERROR,
                                  "Unable to start socket session, because its socket is closed!");

        assert(false);
        return ResultCode::INVARIANTS_VIOLATED;
    }
    else
    {
        WaitForNextMessage ();
        return ResultCode::OK;
    }
}

ResultCode SocketSession::Flush ()
{
    if (!valid_ || outputBuffer_.empty ())
    {
        return ResultCode::OK;
    }

    boost::system::error_code error;
    std::size_t dataToWrite = outputBuffer_.size ();
    std::size_t dataWritten = boost::asio::write (socket_, boost::asio::buffer (outputBuffer_),
                                                  boost::asio::transfer_all (), error);
    outputBuffer_.clear ();

    if (error)
    {
        Evan::Logger::Get ().Log (Evan::LogLevel::ERROR,
                                  "Unable to flush data because of socket io error: " + error.message () +
                                  ". Socket session invalidated!");

        valid_ = false;
        return ResultCode::SOCKET_IO_ERROR;
    }
    else if (dataToWrite != dataWritten)
    {
        Evan::Logger::Get ().Log (
            Evan::LogLevel::ERROR,
            "Unable to flush all data. Tried to flush: " + std::to_string (dataToWrite) +
            ", successfully: " + std::to_string (dataWritten) + "!");
        return ResultCode::UNABLE_TO_FLUSH_ALL_DATA;
    }
    else
    {
        return ResultCode::OK;
    }
}

ResultCode SocketSession::WriteInternal (const MemoryRegion &region)
{
    assert (valid_);
    assert (region.start_);
    assert (region.length_ > 0);

    if (!region.start_)
    {
        return ResultCode::MEMORY_REGION_START_IS_NULLPTR;
    }
    else if (region.length_ == 0)
    {
        return ResultCode::MEMORY_REGION_LENGTH_IS_ZERO;
    }
    else
    {
        std::size_t startIndex = outputBuffer_.size ();
        outputBuffer_.resize (outputBuffer_.size () + region.length_);
        memcpy (&outputBuffer_[startIndex], region.start_, region.length_);
        return ResultCode::OK;
    }
}

void SocketSession::WaitForNextMessage ()
{
    assert (valid_);
    assert (!currentMessageParser_);

    ReadNextChunk (
        sizeof (MessageTypeId),
        [] (SocketSession *$this)
        {
            assert ($this);
            assert ($this->valid_);
            assert (!$this->currentMessageParser_);
            assert ($this->inputBuffer_.size () == sizeof (MessageTypeId));

            MessageTypeId messageType = *(reinterpret_cast<MessageTypeId *>(&$this->inputBuffer_[0]));
            $this->currentMessageParser_ = $this->context_->ConstructParser (messageType);

            if ($this->currentMessageParser_)
            {
                $this->inputBuffer_.clear ();
                $this->InvokeParser ();
            }
            else
            {
                Evan::Logger::Get ().Log (
                    Evan::LogLevel::ERROR,
                    "Unable to find parser for message type " + std::to_string (messageType) +
                    ". Socket session invalidated!");

                $this->valid_ = false;
            }
        });
}

void SocketSession::ReadNextChunk (uint64_t chunkSize, std::function <void (SocketSession *)> onSuccess)
{
    inputBuffer_.resize (chunkSize);
    boost::asio::async_read (
        socket_, boost::asio::buffer (inputBuffer_), boost::asio::transfer_exactly (chunkSize),
        [this, chunkSize, successHandler (std::move (onSuccess))]
            (const boost::system::error_code &error, std::size_t bytesRead) -> void
        {
            if (error)
            {
                Evan::Logger::Get ().Log (
                    Evan::LogLevel::ERROR,
                    "Unable to read next chunk because of socket io error: " + error.message () +
                    ". Socket session invalidated!");
                valid_ = false;
            }
            else if (bytesRead != chunkSize)
            {
                Evan::Logger::Get ().Log (
                    Evan::LogLevel::ERROR,
                    "Unable to read next chink. Tried to read: " + std::to_string (chunkSize) +
                    ", successfully: " + std::to_string (bytesRead) + ". Socket session invalidated!");

                assert (false);
                valid_ = false;
            }
            else
            {
                successHandler (this);
            }
        });
}

void SocketSession::InvokeParser ()
{
    assert (currentMessageParser_);
    MessageParserStatus status = currentMessageParser_ (inputBuffer_, this);

    if (status.valid_)
    {
        if (status.nextChunkSize_ == 0)
        {
            WaitForNextMessage ();
        }
        else
        {
            ReadNextChunk (
                status.nextChunkSize_,
                [] (SocketSession *$this)
                {
                    assert ($this);
                    assert ($this->valid_);
                    assert ($this->currentMessageParser_);
                    $this->InvokeParser ();
                });
        }
    }
    else
    {
        Evan::Logger::Get ().Log (Evan::LogLevel::ERROR,
                                  "Socket session invalidated due to parser internal error!");
        valid_ = false;
    }
}
}
