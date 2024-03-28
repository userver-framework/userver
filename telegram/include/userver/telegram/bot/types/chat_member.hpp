#pragma once

#include <userver/telegram/bot/types/chat_member_administrator.hpp>
#include <userver/telegram/bot/types/chat_member_banned.hpp>
#include <userver/telegram/bot/types/chat_member_left.hpp>
#include <userver/telegram/bot/types/chat_member_member.hpp>
#include <userver/telegram/bot/types/chat_member_owner.hpp>
#include <userver/telegram/bot/types/chat_member_restricted.hpp>

#include <userver/formats/parse/variant.hpp>
#include <userver/formats/json/serialize_variant.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

/// @brief This object contains information about one member of a chat.
/// @see https://core.telegram.org/bots/api#chatmember
using ChatMember = std::variant<std::unique_ptr<ChatMemberOwner>,
                                std::unique_ptr<ChatMemberAdministrator>,
                                std::unique_ptr<ChatMemberMember>,
                                std::unique_ptr<ChatMemberRestricted>,
                                std::unique_ptr<ChatMemberLeft>,
                                std::unique_ptr<ChatMemberBanned>>;

}  // namespace telegram::bot

USERVER_NAMESPACE_END
