#pragma once

#include <userver/telegram/bot/requests/request.hpp>
#include <userver/telegram/bot/types/update.hpp>

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include <userver/formats/json_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

/// @brief Use this method to receive incoming updates using long polling.
/// @note This method will not work if an outgoing webhook is set up. 
/// @note In order to avoid getting duplicate updates, recalculate offset after
/// each server response.
/// @see https://core.telegram.org/bots/api#getupdates
/// @see https://en.wikipedia.org/wiki/Push_technology#Long_polling
struct GetUpdatesMethod {
  static constexpr std::string_view kName = "getUpdates";

  static constexpr auto kHttpMethod = clients::http::HttpMethod::kGet;

  struct Parameters{
    /// @brief Identifier of the first update to be returned.
    /// @note Must be greater by one than the highest among the identifiers of
    /// previously received updates.
    /// @note By default, updates starting with the earliest unconfirmed update
    /// are returned. An update is considered confirmed as soon as GetUpdates is
    /// called with an offset higher than its update_id.
    /// @note The negative offset can be specified to retrieve updates starting
    /// from -offset update from the end of the updates queue.
    /// All previous updates will be forgotten.
    std::optional<std::int64_t> offset;

    /// @brief Limits the number of updates to be retrieved.
    /// Values between 1-100 are accepted. Defaults to 100.
    std::optional<std::int64_t> limit;

    /// @brief Timeout in seconds for long polling. Defaults to 0, i.e. usual
    /// short polling.
    /// @note Should be positive, short polling should be used for testing
    /// purposes only.
    std::optional<std::chrono::seconds> timeout;

    enum class UpdateType {
      kMessage, kEditedMessage, kChannelPost, kEditedChannelPost, kInlineQuery,
      kChosenInlineResult, kCallbackQuery, kShippingQuery, kPreCheckoutQuery,
      kPoll, kPollAnswer, kMyChatMember, kChatMember, kChatJoinRequest
    };

    /// @brief List of the update types you want your bot to receive.
    /// @note Specify an empty list to receive all update types except
    /// chat_member (default).
    /// @note If not specified, the previous setting will be used.
    /// @note Please note that this parameter doesn't affect updates created
    /// before the call to the getUpdates, so unwanted updates may be received
    /// for a short period of time.
    std::optional<std::vector<UpdateType>> allowed_updates;
  };

  /// @brief Returns an Array of Update objects
  using Reply = std::vector<Update>;

  static void FillRequestData(clients::http::Request& request,
                              const Parameters& parameters);

  static Reply ParseResponseData(clients::http::Response& response);
};

std::string_view ToString(GetUpdatesMethod::Parameters::UpdateType update_type);

GetUpdatesMethod::Parameters::UpdateType Parse(
    const formats::json::Value& value,
    formats::parse::To<GetUpdatesMethod::Parameters::UpdateType>);

formats::json::Value Serialize(
    GetUpdatesMethod::Parameters::UpdateType update_type,
    formats::serialize::To<formats::json::Value>);

GetUpdatesMethod::Parameters Parse(const formats::json::Value& json,
                                   formats::parse::To<GetUpdatesMethod::Parameters>);

formats::json::Value Serialize(const GetUpdatesMethod::Parameters& parameters,
                               formats::serialize::To<formats::json::Value>);

using GetUpdatesRequest = Request<GetUpdatesMethod>;

}  // namespace telegram::bot

USERVER_NAMESPACE_END
