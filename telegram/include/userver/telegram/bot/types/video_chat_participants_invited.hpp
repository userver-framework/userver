#pragma once

#include <userver/telegram/bot/types/user.hpp>

#include <vector>

#include <userver/formats/json_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

/// @brief This object represents a service message about
/// new members invited to a video chat.
/// @see https://core.telegram.org/bots/api#videochatparticipantsinvited
struct VideoChatParticipantsInvited {
  /// @brief New members that were invited to the video chat.
  std::vector<User> users;
};

VideoChatParticipantsInvited Parse(const formats::json::Value& json,
                                   formats::parse::To<VideoChatParticipantsInvited>);

formats::json::Value Serialize(const VideoChatParticipantsInvited& video_chat_participants_invited,
                               formats::serialize::To<formats::json::Value>);

}  // namespace telegram::bot

USERVER_NAMESPACE_END
