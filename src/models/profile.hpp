#pragma once

#include <optional>
#include <string>
#include <tuple>

#include <userver/formats/json/value_builder.hpp>

namespace real_medium::models {

struct Profile {
  std::string id;
  std::string username;
  std::optional<std::string> bio;
  std::optional<std::string> image;
  bool following{false};
};

}  // namespace real_medium::models

