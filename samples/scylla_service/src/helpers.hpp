#pragma once

#include <chrono>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <userver/formats/json/value.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/server/http/http_request.hpp>
#include <userver/storages/scylla/operations.hpp>
#include <userver/utest/using_namespace_userver.hpp>

namespace samples::scylladb {

using namespace storages::scylla;

inline constexpr std::string_view kBasicTable = "basic";
inline constexpr std::string_view kEventsTable = "events";
inline constexpr std::string_view kComponentName = "scylla-db";

[[noreturn]] void BadRequest(std::string message);

const std::string& RequireArg(const server::http::HttpRequest& request, std::string_view name);

formats::json::Value ParseBody(const server::http::HttpRequest& request);

formats::json::ValueBuilder ValueToJson(const Value& v);

formats::json::ValueBuilder RowToJson(const Row& row);

formats::json::ValueBuilder RowsToJson(const Rows& rows);

Value JsonToBasicValue(std::string_view name, const formats::json::Value& json);

std::string ToHex(std::string_view data);

std::string FromHex(std::string_view hex);

struct Event {
    Uuid id;
    std::string name;
    Timestamp created_at{};
    std::string source_ip;
    std::vector<std::string> tags;
    std::vector<std::pair<std::string, std::string>> metadata;
    std::vector<std::int32_t> scores;
};

void DecodeRow(const Row& row, Event& out);

formats::json::ValueBuilder EventToJson(const Event& event);

}  // namespace samples::scylladb
