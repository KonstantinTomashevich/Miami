#pragma once

#include <cstdint>
#include <mutex>
#include <vector>

#include <boost/asio/ip/tcp.hpp>

#include <Miami/Annotations.hpp>

#include <Miami/Disco/Context.hpp>

#include <Miami/Hotline/Session.hpp>
#include <Miami/Hotline/Message.hpp>
#include <Miami/Hotline/ResultCode.hpp>

namespace Miami::Hotline
{
class SocketContext;

struct MemoryRegion
{
    const void *start_;
    uint64_t length_;
};

class SocketSession final
{
public:
    SocketSession (Disco::Context *multithreadingContext, SocketContext *socketContext);

    ResultCode Write (const MemoryRegion &region);

    ResultCode Write (const std::vector <MemoryRegion> &regions);

    // TODO: Temporary adhok.
    ResultCode WriteMessage (MessageTypeId type, const MemoryRegion &dataRegion);

    // TODO: Temporary adhok.
    ResultCode WriteMessage (MessageTypeId type, const std::vector <MemoryRegion> &dataRegions);

    Session &Data ();

private:
    free_call ResultCode Start ();

    free_call ResultCode Flush ();

    free_call ResultCode WriteInternal (const MemoryRegion &region);

    free_call void WaitForNextMessage ();

    free_call void ReadNextChunk (uint64_t chunkSize, std::function <void (SocketSession *)> bytesRead);

    free_call void InvokeParser ();

    std::mutex outputBufferGuard_;
    std::vector <uint8_t> outputBuffer_;

    Session session_;
    boost::asio::ip::tcp::socket socket_;
    SocketContext *context_;

    MessageParser currentMessageParser_;
    std::vector <uint8_t> inputBuffer_;
    bool valid_;

    friend class SocketContext;
};
}