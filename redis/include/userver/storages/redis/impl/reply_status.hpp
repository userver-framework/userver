#pragma once

USERVER_NAMESPACE_BEGIN

namespace redis {

/// Redis reply status
enum class ReplyStatus {
  kOk = 0,
  kInputOutputError,
  kOtherError,
  kEndOfFileError,
  kProtocolError,
  kOutOfMemoryError,
  kTimeoutError,
};

}  // namespace redis

USERVER_NAMESPACE_END
