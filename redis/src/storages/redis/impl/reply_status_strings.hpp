#pragma once

#include <iostream>

#include <userver/utils/trivial_map.hpp>

#include <userver/storages/redis/impl/reply_status.hpp>

USERVER_NAMESPACE_BEGIN

namespace redis {

/// String representations of ReplyStatus codes
constexpr utils::TrivialBiMap kReplyStatusMap = [](auto selector) {
  return selector()
      .Case(ReplyStatus::kOk, "OK")
      .Case(ReplyStatus::kInputOutputError, "input_output_error")
      .Case(ReplyStatus::kOtherError, "other_error")
      .Case(ReplyStatus::kEndOfFileError, "EOF")
      .Case(ReplyStatus::kProtocolError, "protocol_error")
      .Case(ReplyStatus::kOutOfMemoryError, "OOM")
      .Case(ReplyStatus::kTimeoutError, "timeout");
};

/// Returns a string representation of a ReplyStatus code
inline auto ToString(ReplyStatus status) {
  return std::string{*kReplyStatusMap.TryFindByFirst(status)};
}

/// Convenience operator for ReplyStatus logging
inline std::ostream& operator<<(std::ostream& ostr, ReplyStatus status) {
  ostr << *kReplyStatusMap.TryFindByFirst(status);
  return ostr;
}

}  // namespace redis

USERVER_NAMESPACE_END
