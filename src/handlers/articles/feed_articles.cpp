#include "feed_articles.hpp"
#include <boost/lexical_cast.hpp>
#include <sstream>
#include <userver/formats/serialize/common_containers.hpp>
#include "db/sql.hpp"
#include "models/article.hpp"

namespace real_medium::handlers::articles::feed::get {

Handler::Handler(const userver::components::ComponentConfig& config,
                 const userver::components::ComponentContext& component_context)
    : HttpHandlerJsonBase(config, component_context),
      pg_cluster_(component_context
                      .FindComponent<userver::components::Postgres>(
                          "realmedium-database")
                      .GetCluster()) {}

userver::formats::json::Value Handler::HandleRequestJsonThrow(
    const userver::server::http::HttpRequest& request,
    const userver::formats::json::Value& request_json,
    userver::server::request::RequestContext& context) const {
  auto user_id = context.GetData<std::string>("id");
  userver::formats::json::ValueBuilder builder;
  builder["articles"] = std::vector<real_medium::models::TaggedArticleWithProfile>();
  builder["articlesCount"] = 0;
  return builder.ExtractValue();
}

}  // namespace real_medium::handlers::articles::feed::get
