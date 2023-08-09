#pragma once

#include <string>
#include <tuple>

#include <userver/formats/json.hpp>
#include <userver/formats/parse/common_containers.hpp>



namespace real_medium::dto {

struct CommentDTO {
  std::string id;
  std::string createdAtl;
  std::string updatedAt;
  std::string body;
  //real_medium::models::Author author; author это тот же профиль
  std::string Author;
};

struct AddCommentDTO {
  std::string body;
};

CommentDTO Parse(const userver::formats::json::Value& json,
           userver::formats::parse::To<CommentDTO>);
           
AddCommentDTO Parse(const userver::formats::json::Value& json,
           userver::formats::parse::To<AddCommentDTO>);

}