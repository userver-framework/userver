#include "filter.hpp"
#include "boost/lexical_cast.hpp"

namespace real_medium::dto {
ArticleFilterDTO Parse(const userver::server::http::HttpRequest& request) {
  ArticleFilterDTO filter;
  if (request.HasArg("tag")) {
    filter.tag = request.GetArg("tag");
  }
  if (request.HasArg("author")) {
    filter.author = request.GetArg("author");
  }
  if (request.HasArg("favorited")) {
    filter.favorited = request.GetArg("favorited");
  }
  if (request.HasArg("limit")) {
    filter.limit = boost::lexical_cast<std::int32_t>(request.GetArg("limit"));
    filter.limit = std::max(0, filter.limit);
  }
  if (request.HasArg("offset")) {
    filter.offset = boost::lexical_cast<std::int32_t>(request.GetArg("offset"));
    filter.offset = std::max(0, filter.offset);
  }
  return filter;
}
}  // namespace real_medium::dto