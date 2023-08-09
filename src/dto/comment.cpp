#include "comment.hpp"
#include "../models/author.hpp"
namespace real_medium::dto {

CommentDTO Parse(const userver::formats::json::Value& json,
                 userver::formats::parse::To<CommentDTO>) {
  return CommentDTO{
      json["id"].As<std::string>(),
      json["createdAt"].As<std::string>(),
      json["updatedAt"].As<std::string>(),
      json["body"].As<std::string>(),
      json["author"].As<real_medium::models::Author>()
  };
}


}
