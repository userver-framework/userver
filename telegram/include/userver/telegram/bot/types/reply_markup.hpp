#pragma once

#include <userver/telegram/bot/types/force_reply.hpp>
#include <userver/telegram/bot/types/inline_keyboard_markup.hpp>
#include <userver/telegram/bot/types/reply_keyboard_markup.hpp>
#include <userver/telegram/bot/types/reply_keyboard_remove.hpp>

#include <variant>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

/// @brief Object for an inline keyboard, custom reply keyboard, instructions
/// to remove reply keyboard or to force a reply from the user.
using ReplyMarkup = std::variant<std::unique_ptr<InlineKeyboardMarkup>,
                                 std::unique_ptr<ReplyKeyboardMarkup>,
                                 std::unique_ptr<ReplyKeyboardRemove>,
                                 std::unique_ptr<ForceReply>>;

}  // namespace telegram::bot

USERVER_NAMESPACE_END
