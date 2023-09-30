#pragma once

#include <cstdint>
#include <string>
#include <variant>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

/// @brief Unique identifier for the chat or username of the channel
/// (in the format @channelusername)
using ChatId = std::variant<std::int64_t, std::string>;

}  // namespace telegram::bot

USERVER_NAMESPACE_END
