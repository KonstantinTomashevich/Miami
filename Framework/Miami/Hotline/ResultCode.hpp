#pragma once

namespace Miami::Hotline
{
enum class ResultCode
{
    OK = 0,
    INVARIANTS_VIOLATED,

    UNABLE_TO_FLUSH_ALL_DATA,
    SOCKET_IO_ERROR,

    MEMORY_REGION_START_IS_NULLPTR,
    MEMORY_REGION_LENGTH_IS_ZERO,
    INVALID_SOCKET_SESSION,

    FACTORY_FOR_MESSAGE_TYPE_IS_ALREADY_REGISTERED,
    FACTORY_CAN_NOT_BE_NULL,
    ENDPOINTS_NOT_FOUND
};
}