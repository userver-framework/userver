#pragma once

#include <userver/telegram/bot/types/callback_query.hpp>
#include <userver/telegram/bot/types/chat_join_request.hpp>
#include <userver/telegram/bot/types/chat_member_updated.hpp>
#include <userver/telegram/bot/types/chosen_inline_result.hpp>
#include <userver/telegram/bot/types/inline_query.hpp>
#include <userver/telegram/bot/types/message.hpp>
#include <userver/telegram/bot/types/poll.hpp>
#include <userver/telegram/bot/types/poll_answer.hpp>
#include <userver/telegram/bot/types/pre_checkout_query.hpp>
#include <userver/telegram/bot/types/shipping_query.hpp>

#include <cstdint>
#include <memory>

#include <userver/formats/json_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

/// @brief This object represents an incoming update.
/// @note At most one of the optional parameters can be present in any
/// given update.
/// @see https://core.telegram.org/bots/api#update
struct Update {
  /// @brief The update's unique identifier. Update identifiers start from a
  /// certain positive number and increase sequentially.
  /// @details This ID becomes especially handy if you're using webhooks, since
  /// it allows you to ignore repeated updates or to restore the correct update
  /// sequence, should they get out of order.
  /// @note If there are no new updates for at least a week, then identifier of
  /// the next update will be chosen randomly instead of sequentially.
  std::int64_t update_id{};

  /// @brief Optional. New incoming message of any kind - text, photo,
  /// sticker, etc.
  std::unique_ptr<Message> message;

  /// @brief Optional. New version of a message that is known to the bot
  /// and was edited.
  std::unique_ptr<Message> edited_message;

  /// @brief Optional. New incoming channel post of any kind - text, photo,
  /// sticker, etc.
  std::unique_ptr<Message> channel_post;

  /// @brief Optional. New version of a channel post that is known to the bot
  /// and was edited.
  std::unique_ptr<Message> edited_channel_post;

  /// @brief Optional. New incoming inline query.
  std::unique_ptr<InlineQuery> inline_query;

  /// @brief Optional. The result of an inline query that was chosen by a user
  /// and sent to their chat partner. Please see our documentation on the
  /// feedback collecting for details on how to enable these updates for
  /// your bot.
  std::unique_ptr<ChosenInlineResult> chosen_inline_result;

  /// @brief Optional. New incoming callback query.
  std::unique_ptr<CallbackQuery> callback_query;

  /// @brief Optional. New incoming shipping query.
  /// @note Only for invoices with flexible price.
  std::unique_ptr<ShippingQuery> shipping_query;

  /// @brief Optional. New incoming pre-checkout query. Contains full
  /// information about checkout.
  std::unique_ptr<PreCheckoutQuery> pre_checkout_query;

  /// @brief Optional. New poll state. Bots receive only updates about
  /// stopped polls and polls, which are sent by the bot.
  std::unique_ptr<Poll> poll;

  /// @brief Optional. A user changed their answer in a non-anonymous poll.
  /// @note Bots receive new votes only in polls that were sent by the
  /// bot itself.
  std::unique_ptr<PollAnswer> poll_answer;

  /// @brief Optional. The bot's chat member status was updated in a chat.
  /// @note For private chats, this update is received only when the bot is
  /// blocked or unblocked by the user.
  std::unique_ptr<ChatMemberUpdated> my_chat_member;

  /// @brief Optional. A chat member's status was updated in a chat.
  /// @note The bot must be an administrator in the chat and must explicitly
  /// specify “chat_member” in the list of allowed_updates to
  /// receive these updates.
  std::unique_ptr<ChatMemberUpdated> chat_member;

  /// @brief Optional. A request to join the chat has been sent.
  /// @note The bot must have the can_invite_users administrator right in the
  /// chat to receive these updates.
  std::unique_ptr<ChatJoinRequest> chat_join_request;
};

Update Parse(const formats::json::Value& json,
             formats::parse::To<Update>);

formats::json::Value Serialize(const Update& update,
                               formats::serialize::To<formats::json::Value>);

}  // namespace telegram::bot

USERVER_NAMESPACE_END
