#pragma once

#include <cstdint>
#include <string>

#include <userver/formats/json_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

/// @brief This object contains information about one answer option in a poll.
/// @see https://core.telegram.org/bots/api#polloption
struct PollOption {
  /// @brief Option text, 1-100 characters.
  std::string text;

  /// @brief Number of users that voted for this option.
  std::int64_t voter_count{};
};

PollOption Parse(const formats::json::Value& json,
                 formats::parse::To<PollOption>);

formats::json::Value Serialize(const PollOption& poll_option,
                               formats::serialize::To<formats::json::Value>);

}  // namespace telegram::bot

USERVER_NAMESPACE_END
