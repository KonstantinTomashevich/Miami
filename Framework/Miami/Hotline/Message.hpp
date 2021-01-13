#pragma once

#include <cstdint>
#include <functional>

namespace Miami::Hotline
{
using MessageTypeId = uint16_t;

struct MessageParserStatus
{
    uint64_t nextChunkSize_;
};

using MessageParser = std::function <MessageParserStatus (const std::vector &<uint8_t>, SocketSession *)>;

using MessageParserFactory = std::function <MessageParser ()>;
}