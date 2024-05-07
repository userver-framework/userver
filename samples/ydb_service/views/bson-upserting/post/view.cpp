#include "view.hpp"

#include <userver/ydb/component.hpp>
#include <userver/ydb/table.hpp>

namespace sample {

namespace {
static const std::string kInsertQuery = R"(
--!syntax_v1
DECLARE $id AS String;
DECLARE $doc AS String;

UPSERT INTO orders (id, doc)
VALUES ($id, $doc);
)";
}

std::string BsonUpsertingHandler::HandleRequestThrow(
    const server::http::HttpRequest& request,
    server::request::RequestContext&) const {
  const auto& id = request.GetArg("id");
  const auto& body = request.RequestBody();

  ydb_client_->ExecuteDataQuery(kInsertQuery, "$id", id, "$doc", body);
  return {};
}

}  // namespace sample
