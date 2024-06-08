#include "view.hpp"

#include <userver/engine/sleep.hpp>
#include <userver/formats/json.hpp>
#include <userver/logging/log.hpp>
#include <userver/utest/using_namespace_userver.hpp>
#include <userver/utils/datetime.hpp>

#include <userver/ydb/table.hpp>

namespace {

const ydb::Query kUpsertQuery{
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

}  // namespace

namespace sample {

formats::json::Value UpsertRowHandler::HandleRequestJsonThrow(
    const server::http::HttpRequest&, const formats::json::Value& request,
    server::request::RequestContext&) const {
  engine::SleepFor(std::chrono::milliseconds(10));

  auto trx = Ydb().Begin("trx", ydb::TransactionMode::kSerializableRW);
  auto response =
      trx.Execute(kUpsertQuery,                                               //
                  "$id_key", request["id"].As<std::string>(),                 //
                  "$name_key", ydb::Utf8{request["name"].As<std::string>()},  //
                  "$service_key", request["service"].As<std::string>(),       //
                  "$channel_key", request["channel"].As<int64_t>(),           //
                  "$state_key",
                  request["state"].As<std::optional<formats::json::Value>>()  //
      );

  if (response.GetCursorCount()) {
    throw std::runtime_error("Unexpected response data");
  }

  trx.Commit();

  return formats::json::MakeObject();
}

}  // namespace sample
