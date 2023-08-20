#pragma once

#include <string>
#include <tuple>

#include <userver/formats/json.hpp>
#include <userver/formats/parse/common_containers.hpp>

namespace real_medium::dto {

struct AddCommentDTO {
  std::optional<std::string> body;
};

AddCommentDTO Parse(const userver::formats::json::Value& json,
                    userver::formats::parse::To<AddCommentDTO>);

}  // namespace real_medium::dto