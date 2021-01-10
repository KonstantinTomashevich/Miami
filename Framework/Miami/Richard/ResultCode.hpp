#pragma once

namespace Miami::Richard
{
enum class ResultCode
{
    OK = 0,
    INVARIANTS_VIOLATED,

    CURSOR_ADVANCE_STOPPED_AT_BEGIN,
    CURSOR_ADVANCE_STOPPED_AT_END,

    CURSOR_GET_CURRENT_UNABLE_TO_GET_FROM_END,

    COLUMN_WITH_GIVEN_ID_NOT_FOUND,
    INDEX_WITH_GIVEN_ID_NOT_FOUND,
    ROW_WITH_GIVEN_ID_NOT_FOUND,

    TABLE_NAME_SHOULD_NOT_BE_EMPTY,
    COLUMN_NAME_SHOULD_NOT_BE_EMPTY,

    INDEX_NAME_SHOULD_NOT_BE_EMPTY,
    INDEX_MUST_DEPEND_ON_AT_LEAST_ONE_COLUMN,

    COLUMN_REMOVAL_BLOCKED_BY_DEPENDANT_INDEX,
    INDEX_REMOVAL_BLOCKED_BY_DEPENDANT_CURSORS,
    NEW_COLUMN_VALUE_TYPE_MISMATCH,
};
}