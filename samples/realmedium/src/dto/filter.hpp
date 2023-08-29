#pragma once

#include <optional>
#include <string>
#include <tuple>

#include <userver/formats/json.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/server/http/http_request.hpp>
namespace real_medium::dto {

struct ArticleFilterDTO {
  std::optional<std::string> tag;
  std::optional<std::string> author;
  std::optional<std::string> favorited;
  std::int32_t limit = 20;
  std::int32_t offset = 0;
};

struct FeedArticleFilterDTO {
  std::int32_t limit = 20;
  std::int32_t offset = 0;
};

template <typename T>
T Parse(const userver::server::http::HttpRequest& request);
}  // namespace real_medium::dto
