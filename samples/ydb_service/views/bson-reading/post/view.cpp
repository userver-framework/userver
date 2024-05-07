#include "view.hpp"

#include <userver/formats/bson.hpp>

#include <userver/ydb/component.hpp>
#include <userver/ydb/table.hpp>

namespace sample {

std::string BsonReadingHandler::HandleRequestThrow(
    const server::http::HttpRequest& request,
    server::request::RequestContext&) const {
  static const std::string kSelectQuery = R"(
--!syntax_v1
DECLARE $id AS String;
SELECT doc FROM orders WHERE id = $id;
  )";
  auto response =
      ydb_client_->ExecuteDataQuery(kSelectQuery, "$id", request.GetArg("id"));

  if (response.GetCursorCount() != 1) {
    throw std::runtime_error("Unexpected response data");
  }
  auto cursor = response.GetSingleCursor();

  if (!cursor.empty()) {
    auto row = cursor.GetFirstRow();

    return formats::bson::ToBinaryString(
               formats::bson::MakeDoc("doc", formats::bson::FromBinaryString(
                                                 row.Get<std::string>("doc"))))
        .ToString();
  }

  request.SetResponseStatus(server::http::HttpStatus::kNotFound);
  return {};
}

}  // namespace sample
