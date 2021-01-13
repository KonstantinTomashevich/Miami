#pragma once

#include <cstdint>
#include <vector>
#include <functional>

namespace Miami::Hotline
{
class SocketSession;

using MessageTypeId = uint16_t;

struct MessageParserStatus
{
    /// Next chunk size or zero if parsing is finished.
    uint64_t nextChunkSize_;

    bool valid_;
};

using MessageParser = std::function <MessageParserStatus (const std::vector <uint8_t> &, SocketSession *)>;

using MessageParserFactory = std::function <MessageParser ()>;
}