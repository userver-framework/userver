#pragma once

/// @file userver/websocket/message.hpp
/// @brief @copybrief websocket::Message

#include <optional>
#include <string>

USERVER_NAMESPACE_BEGIN

namespace websocket {

using CloseStatusInt = int16_t;

/// @brief Close statuses
enum class CloseStatus : CloseStatusInt {
    kNone = 0,

    kNormal = 1000,
    kGoingAway = 1001,
    kProtocolError = 1002,
    kUnsupportedData = 1003,
    kFrameTooLarge = 1004,
    kNoStatusRcvd = 1005,
    kAbnormalClosure = 1006,
    kBadMessageData = 1007,
    kPolicyViolation = 1008,
    kTooBigData = 1009,
    kExtensionMismatch = 1010,
    kServerError = 1011
};

/// @brief WebSocket message
struct Message {
    std::string data;                              ///< payload
    std::optional<CloseStatus> close_status = {};  ///< close status
    bool is_text = false;                          ///< is it text or binary?
};

}  // namespace websocket

USERVER_NAMESPACE_END
