#include <userver/telegram/bot/types/video_chat_participants_invited.hpp>

#include <userver/telegram/bot/types/user.hpp>

#include <userver/formats/json.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/formats/serialize/common_containers.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

namespace impl {

template <class Value>
VideoChatParticipantsInvited Parse(const Value& data,
                                   formats::parse::To<VideoChatParticipantsInvited>) {
  return VideoChatParticipantsInvited{
    data["users"].template As<std::vector<User>>()
  };
}

template <class Value>
Value Serialize(const VideoChatParticipantsInvited& video_chat_participants_invited,
                formats::serialize::To<Value>) {
  typename Value::Builder builder;
  builder["users"] = video_chat_participants_invited.users;
  return builder.ExtractValue();
}

}  // namespace impl

VideoChatParticipantsInvited Parse(const formats::json::Value& json,
                                   formats::parse::To<VideoChatParticipantsInvited> to) {
  return impl::Parse(json, to);
}

formats::json::Value Serialize(const VideoChatParticipantsInvited& video_chat_participants_invited,
                               formats::serialize::To<formats::json::Value> to) {
  return impl::Serialize(video_chat_participants_invited, to);
}

}  // namespace telegram::bot

USERVER_NAMESPACE_END
