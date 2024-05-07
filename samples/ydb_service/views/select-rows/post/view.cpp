#include "view.hpp"

#include <fmt/format.h>

#include <userver/formats/json.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/datetime.hpp>

#include <userver/ydb/component.hpp>
#include <userver/ydb/table.hpp>

namespace sample {

formats::json::Value SelectRowsHandler::HandleRequestJsonThrow(
    const server::http::HttpRequest&, const formats::json::Value& request_json,
    server::request::RequestContext&) const {
  ydb::OperationSettings query_params = {
      3,                                // retries
      std::chrono::milliseconds(1000),  // operation_timeout
      std::chrono::milliseconds(1000),  // cancel_after
      std::chrono::milliseconds(1100),  // client_timeout
      ydb::TransactionMode::kStaleRO};
  static const ydb::Query kSelectQuery = {
      "--!syntax_v1\n"
      "DECLARE $service_key AS String;"
      "DECLARE $channel_keys AS List<Int64>;"
      "DECLARE $created_key AS Timestamp;"
      "SELECT id, name, service, channel, created, state "
      "FROM events VIEW sample_index "
      "WHERE service = $service_key AND channel IN $channel_keys "
      "AND created > $created_key;",
      ydb::Query::Name("select")};

  auto response = Ydb().ExecuteDataQuery(
      query_params, kSelectQuery,                                            //
      "$service_key", request_json["service"].As<std::string>(),             //
      "$channel_keys", request_json["channels"].As<std::vector<int64_t>>(),  //
      "$created_key",
      request_json["created"].As<std::chrono::system_clock::time_point>());

  if (response.GetCursorCount() != 1) {
    throw std::runtime_error("Unexpected response data");
  }

  formats::json::ValueBuilder items_builder(formats::json::Type::kArray);
  for (auto row : std::move(response).ExtractSingleCursor()) {
    formats::json::ValueBuilder item;
    item["id"] = row.Get<std::string>("id");
    item["name"] = row.Get<ydb::Utf8>("name").GetUnderlying();
    item["service"] = row.Get<std::string>("service");
    item["channel"] = row.Get<std::int64_t>("channel");
    item["created"] = row.Get<std::chrono::system_clock::time_point>("created");

    auto state = row.Get<std::optional<formats::json::Value>>("state");
    if (state) {
      item["state"] = std::move(*state);
    }

    items_builder.PushBack(item.ExtractValue());
  }

  return formats::json::MakeObject("items", items_builder.ExtractValue());
}

}  // namespace sample
