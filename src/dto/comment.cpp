#include "comment.hpp"

namespace real_medium::dto {

AddCommentDTO Parse(const userver::formats::json::Value& json,
                    userver::formats::parse::To<AddCommentDTO>) {
  return AddCommentDTO{
      json["body"].As<std::optional<std::string>>(),
  };
}

}  // namespace real_medium::dto
