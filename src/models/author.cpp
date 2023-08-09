#include "author.hpp"

namespace real_medium::models {

Author Parse(const userver::formats::json::Value& json,
                 userver::formats::parse::To<Author>) {
  return Author{
      json["username"].As<std::string>(),
      json["bio"].As<std::string>(),
      json["image"].As<std::string>(),
      json["following"].As<std::string>()
  };
}


}
