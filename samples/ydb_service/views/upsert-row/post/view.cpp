#include "view.hpp"

#include <userver/formats/json.hpp>
#include <userver/ydb/table.hpp>

namespace sample {

formats::json::Value UpsertRowHandler::HandleRequestJsonThrow(
    const server::http::HttpRequest&, const formats::json::Value& request,
    server::request::RequestContext&) const {
  static const ydb::Query kUpsertQuery{
      R"(
--!syntax_v1
DECLARE $id_key AS String;
DECLARE $name_key AS Utf8;
DECLARE $service_key AS String;
DECLARE $channel_key AS Int64;
DECLARE $state_key AS Json?;

UPSERT INTO events (id, name, service, channel, created, state)
VALUES ($id_key, $name_key, $service_key, $channel_key, CurrentUtcTimestamp(), $state_key);
      )",
      ydb::Query::Name{"upsert-row"},
  };

  auto response = Ydb().ExecuteDataQuery(
      kUpsertQuery,                                  //
      "$id_key", request["id"].As<std::string>(),    //
      "$name_key", request["name"].As<ydb::Utf8>(),  //
      "$service_key",
      request["service"].As<std::string>(),  //
      "$channel_key",
      request["channel"].As<std::int64_t>(),  //
      "$state_key",
      request["state"].As<std::optional<formats::json::Value>>()  //
  );

  if (response.GetCursorCount()) {
    throw std::runtime_error("Unexpected response data");
  }

  return formats::json::MakeObject();
}

}  // namespace sample
