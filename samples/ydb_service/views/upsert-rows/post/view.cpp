#include "view.hpp"

#include <userver/ydb/io/structs.hpp>
#include <userver/ydb/table.hpp>

namespace sample {

struct UpsertRowsRequest {
  static constexpr ydb::StructMemberNames kYdbMemberNames{};

  std::string id;
  ydb::Utf8 name;
  std::string service;
  std::optional<int64_t> channel;
};

UpsertRowsRequest Parse(const formats::json::Value& json,
                        formats::parse::To<UpsertRowsRequest>) {
  UpsertRowsRequest request;

  request.id = json["id"].As<std::string>();
  request.name = ydb::Utf8{json["name"].As<std::string>()};
  request.service = json["service"].As<std::string>();
  request.channel = json["channel"].As<std::optional<int64_t>>();

  return request;
}

formats::json::Value UpsertRowsHandler::HandleRequestJsonThrow(
    const server::http::HttpRequest&, const formats::json::Value& request_json,
    server::request::RequestContext&) const {
  static const ydb::Query kUpsertQuery{
      R"(
--!syntax_v1
DECLARE $items AS List<Struct<'id': String, 'name': Utf8, 'service':
                       String, 'channel': Int64?>>;
UPSERT INTO events (id, name, service, channel, created)
SELECT id, name, service, channel, CurrentUtcTimestamp() FROM AS_TABLE($items);
      )",
      ydb::Query::Name{"upsert-rows"},
  };

  const auto upsert_rows =
      request_json["items"].As<std::vector<UpsertRowsRequest>>();

  const auto response =
      Ydb().ExecuteDataQuery(kUpsertQuery, "$items", upsert_rows);

  if (response.GetCursorCount()) {
    throw std::runtime_error("Unexpected response data");
  }

  return formats::json::MakeObject();
}

}  // namespace sample
