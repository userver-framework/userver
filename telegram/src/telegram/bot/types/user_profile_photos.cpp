#include <userver/telegram/bot/types/user_profile_photos.hpp>

#include <userver/formats/json.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/formats/serialize/common_containers.hpp>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

namespace impl {

template <class Value>
UserProfilePhotos Parse(const Value& data,
                        formats::parse::To<UserProfilePhotos>) {
  return UserProfilePhotos{
    data["total_count"].template As<std::int64_t>(),
    data["photos"].template As<std::vector<std::vector<PhotoSize>>>()
  };
}

template <class Value>
Value Serialize(const UserProfilePhotos& user_profile_photos,
                formats::serialize::To<Value>) {
  typename Value::Builder builder;
  builder["total_count"] = user_profile_photos.total_count;
  builder["photos"] = user_profile_photos.photos;
  return builder.ExtractValue();
}

}  // namespace impl

UserProfilePhotos Parse(const formats::json::Value& json,
                        formats::parse::To<UserProfilePhotos> to) {
  return impl::Parse(json, to);
}

formats::json::Value Serialize(const UserProfilePhotos& user_profile_photos,
                               formats::serialize::To<formats::json::Value> to) {
  return impl::Serialize(user_profile_photos, to);
}

}  // namespace telegram::bot

USERVER_NAMESPACE_END
