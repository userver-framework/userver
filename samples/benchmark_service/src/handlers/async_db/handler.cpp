#include "handler.hpp"
#include <userver/utest/using_namespace_userver.hpp>

#include <charconv>
#include <cstdint>

#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/formats/json/string_builder.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/http/common_headers.hpp>
#include <userver/storages/postgres/cluster.hpp>
#include <userver/storages/postgres/component.hpp>
#include <userver/storages/postgres/result_set.hpp>

namespace userver_httparena::async_db {
namespace {
struct ItemRow {
    int64_t id;
    std::string_view name;
    std::string_view category;
    int64_t price;
    int64_t quantity;
    bool active;
    formats::json::Value tags;
    int64_t rating_score;
    int64_t rating_count;
};
}  // namespace

Handler::Handler(const components::ComponentConfig& config, const components::ComponentContext& context)
    : HttpHandlerBase(config, context),
      pg_{context.FindComponent<components::Postgres>("hello-world-db").GetCluster()}
{}

std::string Handler::HandleRequestThrow(const server::http::HttpRequest& request, server::request::RequestContext&)
    const {
    const auto& min_str = request.GetArg("min");
    const auto& max_str = request.GetArg("max");
    const auto& limit_str = request.GetArg("limit");

    auto min = 10;
    auto max = 50;
    auto limit = 50;
    if (!min_str.empty()) {
        std::from_chars(min_str.data(), min_str.data() + min_str.size(), min);
    }
    if (!max_str.empty()) {
        std::from_chars(max_str.data(), max_str.data() + max_str.size(), max);
    }
    if (!limit_str.empty()) {
        std::from_chars(limit_str.data(), limit_str.data() + limit_str.size(), limit);
    }

    // Matches
    // https://www.http-arena.com/docs/test-profiles/h1/isolated/async-database/implementation/
    auto res = pg_->Execute(
        storages::postgres::ClusterHostType::kSlaveOrMaster,
        "SELECT id, name, category, price, quantity, active, tags, "
        "rating_score, rating_count FROM items "
        "WHERE price BETWEEN $1 AND $2 LIMIT $3",
        min,
        max,
        limit
    );

    auto items = res.AsSetOf<ItemRow>(storages::postgres::kRowTag);

    formats::json::StringBuilder sb;
    {
        formats::json::StringBuilder::ObjectGuard root_guard(sb);
        sb.Key("count");
        sb.WriteInt64(static_cast<int64_t>(items.Size()));
        sb.Key("items");
        {
            formats::json::StringBuilder::ArrayGuard items_guard(sb);
            for (const auto& row : items) {
                formats::json::StringBuilder::ObjectGuard item_guard(sb);
                sb.Key("id");
                sb.WriteInt64(row.id);
                sb.Key("name");
                sb.WriteString(row.name);
                sb.Key("category");
                sb.WriteString(row.category);
                sb.Key("price");
                sb.WriteInt64(row.price);
                sb.Key("quantity");
                sb.WriteInt64(row.quantity);
                sb.Key("active");
                sb.WriteBool(row.active);
                sb.Key("tags");
                sb.WriteValue(row.tags);
                sb.Key("rating");
                {
                    formats::json::StringBuilder::ObjectGuard rating_guard(sb);
                    sb.Key("score");
                    sb.WriteInt64(row.rating_score);
                    sb.Key("count");
                    sb.WriteInt64(row.rating_count);
                }
            }
        }
    }

    request.GetHttpResponse().SetHeader(http::headers::kContentType, "application/json");
    return sb.GetString();
}
}  // namespace userver_httparena::async_db
