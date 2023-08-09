#pragma once

#include <string>
#include <tuple>

#include <userver/formats/json.hpp>
#include <userver/formats/parse/common_containers.hpp>

namespace real_medium::models {

struct Author {
  std::string username;
  std::string bio;
  std::string image;
  std::string following;
};

Author Parse(const userver::formats::json::Value& json,
           userver::formats::parse::To<Author>);


}