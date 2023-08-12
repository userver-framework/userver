#include "profile.hpp"


namespace real_medium::dto {

userver::formats::json::Value Serialize(
    const real_medium::dto::Profile& dto,
    userver::formats::serialize::To<userver::formats::json::Value>) {
  userver::formats::json::ValueBuilder builder;
  builder["username"] = dto.username;
  if(dto.bio)
    builder["bio"]=*dto.bio;
  else
    builder["bio"]=userver::formats::common::Type::kNull;
  if(dto.image)
    builder["image"]=*dto.image;
  else
    builder["image"]=userver::formats::common::Type::kNull;;
  builder["following"] = dto.isFollowing;
  return builder.ExtractValue();
}

}  // realmedium::dto
