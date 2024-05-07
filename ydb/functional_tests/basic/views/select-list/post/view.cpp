#include "view.hpp"

#include <fmt/format.h>

#include <userver/formats/json.hpp>
#include <userver/formats/serialize/common_containers.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/datetime.hpp>

#include <userver/ydb/component.hpp>
#include <userver/ydb/table.hpp>

namespace sample {

formats::json::Value SelectListHandler::HandleRequestJsonThrow(
    const server::http::HttpRequest&, const formats::json::Value&,
    server::request::RequestContext&) const {
  ydb::OperationSettings query_params = {
      3,                                // retries
      std::chrono::milliseconds(1000),  // operation_timeout
      std::chrono::milliseconds(1000),  // cancel_after
      std::chrono::milliseconds(1100),  // client_timeout
      ydb::TransactionMode::kStaleRO};
  static const ydb::Query kSelectQuery = {
      "--!syntax_v1\n"
      "SELECT AGGREGATE_LIST(channel) as channels, "
      "AGGREGATE_LIST(name) as names FROM events;",
      ydb::Query::Name("select")};

  auto response = Ydb().ExecuteDataQuery(query_params, kSelectQuery);

  if (response.GetCursorCount() != 1) {
    throw std::runtime_error("Unexpected response data");
  }
  auto cursor = response.GetSingleCursor();
  if (cursor.IsTruncated()) {
    throw std::runtime_error("Truncated response data");
  }
  auto row = cursor.GetFirstRow();

  auto channels = row.Get<std::vector<std::int64_t>>("channels");
  auto names = row.Get<std::vector<ydb::Utf8>>("names");

  return formats::json::MakeObject(
      "channels", formats::json::ValueBuilder{channels}.ExtractValue(), "names",
      formats::json::ValueBuilder{names}.ExtractValue());
}

}  // namespace sample
