#pragma once

/// @file
/// @brief @copybrief storages::redis::ReplyStatus

USERVER_NAMESPACE_BEGIN

namespace storages::redis {

/// @brief Valkey/Redis reply status
enum class ReplyStatus {
    kOk = 0,
    kInputOutputError,
    kOtherError,
    kEndOfFileError,
    kProtocolError,
    kOutOfMemoryError,
    kTimeoutError,
};

}  // namespace storages::redis

USERVER_NAMESPACE_END
