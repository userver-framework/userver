#include <chrono>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <fmt/format.h>

#include <userver/clients/dns/component.hpp>
#include <userver/components/component.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/formats/json/inline.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/storages/scylla/component.hpp>
#include <userver/storages/scylla/operations.hpp>
#include <userver/storages/scylla/session.hpp>
#include <userver/storages/secdist/component.hpp>
#include <userver/storages/secdist/provider_component.hpp>
#include <userver/utils/daemon_run.hpp>
#include <userver/utest/using_namespace_userver.hpp>

#include "helpers.hpp"

namespace samples::scylladb {
namespace {

using Builder = formats::json::ValueBuilder;

Builder JsonObject() { return Builder{formats::common::Type::kObject}; }

constexpr std::string_view kOptionalFields[] = {"bln", "i32", "i64", "flt", "dbl"};

bool IsKnownOptionalField(std::string_view name) {
    for (auto known : kOptionalFields) {
        if (name == known) {
            return true;
        }
    }
    return false;
}

template <typename Op>
void BindBasicFields(Op& op, const formats::json::Value& json) {
    for (auto name : kOptionalFields) {
        if (json.HasMember(name)) {
            op.Bind(std::string{name}, JsonToBasicValue(name, json[name]));
        }
    }
}

class KeyValueHandler final : public server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-kv";

    KeyValueHandler(const components::ComponentConfig& config, const components::ComponentContext& context)
        : HttpHandlerBase(config, context),
          session_(context.FindComponent<components::Scylla>(kComponentName).GetSession()) {}

    std::string HandleRequest(server::http::HttpRequest& request, server::request::RequestContext&) const override {
        request.GetHttpResponse().SetContentType(http::content_type::kApplicationJson);
        switch (request.GetMethod()) {
            case server::http::HttpMethod::kGet:
                return Get(request);
            case server::http::HttpMethod::kPost:
                return Post(request);
            case server::http::HttpMethod::kPatch:
                return Patch(request);
            case server::http::HttpMethod::kDelete:
                return Delete(request);
            default:
                BadRequest(fmt::format("Unsupported method {}", request.GetMethod()));
        }
    }

private:
    std::string Get(server::http::HttpRequest& request) const {
        operations::SelectOne op;
        op.AddAllColumns();
        op.WhereString("key", RequireArg(request, "key"));
        auto row = session_->GetTable(std::string{kBasicTable}).Execute(op);
        if (row.Empty()) {
            request.SetResponseStatus(server::http::HttpStatus::kNotFound);
            return R"({"error":"not_found"})";
        }
        return formats::json::ToString(RowToJson(row).ExtractValue());
    }

    std::string Post(const server::http::HttpRequest& request) const {
        const auto body = ParseBody(request);
        if (!body.HasMember("key")) {
            BadRequest("JSON field 'key' is required");
        }
        operations::InsertOne op;
        op.BindString("key", body["key"].As<std::string>());
        BindBasicFields(op, body);
        if (body.HasMember("ttl")) {
            op.UsingTtl(body["ttl"].As<std::int32_t>());
        }
        session_->GetTable(std::string{kBasicTable}).Execute(op);
        return R"({"status":"ok"})";
    }

    std::string Patch(const server::http::HttpRequest& request) const {
        const auto body = ParseBody(request);
        operations::UpdateOne op;
        for (auto name : kOptionalFields) {
            if (body.HasMember(name)) {
                op.Set(std::string{name}, JsonToBasicValue(name, body[name]));
            }
        }
        op.WhereString("key", RequireArg(request, "key"));
        if (body.HasMember("ttl")) {
            op.UsingTtl(body["ttl"].As<std::int32_t>());
        }
        session_->GetTable(std::string{kBasicTable}).Execute(op);
        return R"({"status":"ok"})";
    }

    std::string Delete(const server::http::HttpRequest& request) const {
        operations::DeleteOne op;
        op.WhereString("key", RequireArg(request, "key"));
        session_->GetTable(std::string{kBasicTable}).Execute(op);
        return R"({"status":"ok"})";
    }

    SessionPtr session_;
};

class ListHandler final : public server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-kv-list";

    ListHandler(const components::ComponentConfig& config, const components::ComponentContext& context)
        : HttpHandlerBase(config, context),
          session_(context.FindComponent<components::Scylla>(kComponentName).GetSession()) {}

    std::string HandleRequest(server::http::HttpRequest& request, server::request::RequestContext&) const override {
        request.GetHttpResponse().SetContentType(http::content_type::kApplicationJson);
        operations::SelectMany op;
        op.AddAllColumns();
        if (request.HasArg("limit")) {
            op.SetLimit(std::stoul(request.GetArg("limit")));
        }
        const auto rows = session_->GetTable(std::string{kBasicTable}).Execute(op);
        auto response = JsonObject();
        response["items"] = RowsToJson(rows);
        response["count"] = static_cast<std::int64_t>(rows.size());
        return formats::json::ToString(response.ExtractValue());
    }

private:
    SessionPtr session_;
};

class CountHandler final : public server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-kv-count";

    CountHandler(const components::ComponentConfig& config, const components::ComponentContext& context)
        : HttpHandlerBase(config, context),
          session_(context.FindComponent<components::Scylla>(kComponentName).GetSession()) {}

    std::string HandleRequest(server::http::HttpRequest& request, server::request::RequestContext&) const override {
        request.GetHttpResponse().SetContentType(http::content_type::kApplicationJson);
        operations::Count op;
        if (request.HasArg("key")) {
            op.WhereString("key", request.GetArg("key"));
        }
        const auto count = session_->GetTable(std::string{kBasicTable}).Execute(op);
        return formats::json::ToString(formats::json::MakeObject("count", count));
    }

private:
    SessionPtr session_;
};

class BulkInsertHandler final : public server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-kv-bulk";

    BulkInsertHandler(const components::ComponentConfig& config, const components::ComponentContext& context)
        : HttpHandlerBase(config, context),
          session_(context.FindComponent<components::Scylla>(kComponentName).GetSession()) {}

    std::string HandleRequest(server::http::HttpRequest& request, server::request::RequestContext&) const override {
        request.GetHttpResponse().SetContentType(http::content_type::kApplicationJson);
        const auto body = ParseBody(request);
        if (!body.IsArray() || body.GetSize() == 0) {
            BadRequest("Body must be a non-empty JSON array");
        }

        operations::InsertMany op;
        for (std::size_t i = 0; i < body.GetSize(); ++i) {
            const auto& row = body[i];
            if (i > 0) {
                op.NextRow();
            }
            op.BindString("key", row["key"].As<std::string>());
            BindBasicFields(op, row);
        }
        session_->GetTable(std::string{kBasicTable}).Execute(op);
        return formats::json::ToString(
            formats::json::MakeObject("inserted", static_cast<std::int64_t>(body.GetSize()))
        );
    }

private:
    SessionPtr session_;
};

class PaginatedHandler final : public server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-kv-pages";

    PaginatedHandler(const components::ComponentConfig& config, const components::ComponentContext& context)
        : HttpHandlerBase(config, context),
          session_(context.FindComponent<components::Scylla>(kComponentName).GetSession()) {}

    std::string HandleRequest(server::http::HttpRequest& request, server::request::RequestContext&) const override {
        request.GetHttpResponse().SetContentType(http::content_type::kApplicationJson);

        operations::SelectMany op;
        op.AddAllColumns();

        std::size_t page_size = 10;
        if (request.HasArg("page_size")) {
            page_size = std::stoul(request.GetArg("page_size"));
        }
        op.SetPageSize(page_size);

        std::string cursor;
        if (request.HasArg("cursor")) {
            cursor = FromHex(request.GetArg("cursor"));
        }

        auto result = session_->GetTable(std::string{kBasicTable}).ExecutePaged(op, std::move(cursor));

        auto response = JsonObject();
        response["items"] = RowsToJson(result.rows);
        response["count"] = static_cast<std::int64_t>(result.rows.size());
        response["has_more"] = result.has_more_pages;
        if (result.has_more_pages) {
            response["next_cursor"] = ToHex(result.paging_state);
        }
        return formats::json::ToString(response.ExtractValue());
    }

private:
    SessionPtr session_;
};

class CreateIfAbsentHandler final : public server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-kv-create-if-absent";

    CreateIfAbsentHandler(const components::ComponentConfig& config, const components::ComponentContext& context)
        : HttpHandlerBase(config, context),
          session_(context.FindComponent<components::Scylla>(kComponentName).GetSession()) {}

    std::string HandleRequest(server::http::HttpRequest& request, server::request::RequestContext&) const override {
        request.GetHttpResponse().SetContentType(http::content_type::kApplicationJson);
        const auto body = ParseBody(request);
        if (!body.HasMember("key")) {
            BadRequest("JSON field 'key' is required");
        }
        operations::InsertOne op;
        op.BindString("key", body["key"].As<std::string>());
        BindBasicFields(op, body);
        op.IfNotExists();
        const auto result = session_->GetTable(std::string{kBasicTable}).ExecuteLwt(op);
        auto response = JsonObject();
        response["applied"] = result.applied;
        if (!result.applied) {
            request.SetResponseStatus(server::http::HttpStatus::kConflict);
            response["existing"] = RowToJson(result.previous);
        }
        return formats::json::ToString(response.ExtractValue());
    }

private:
    SessionPtr session_;
};

class CompareAndSetHandler final : public server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-kv-cas";

    CompareAndSetHandler(const components::ComponentConfig& config, const components::ComponentContext& context)
        : HttpHandlerBase(config, context),
          session_(context.FindComponent<components::Scylla>(kComponentName).GetSession()) {}

    std::string HandleRequest(server::http::HttpRequest& request, server::request::RequestContext&) const override {
        request.GetHttpResponse().SetContentType(http::content_type::kApplicationJson);
        const auto body = ParseBody(request);
        if (!body.HasMember("expect") || !body.HasMember("set")) {
            BadRequest("JSON fields 'expect' and 'set' are required");
        }
        const auto& expect = body["expect"];
        const auto& set = body["set"];

        operations::UpdateOne op;
        for (const auto [name, value] : Items(set)) {
            if (!IsKnownOptionalField(name)) {
                BadRequest(
                    fmt::format(
                        "field '{}' is not allowed in 'set' (allowed: bln, "
                        "i32, i64, flt, dbl)",
                        name
                    )
                );
            }
            op.Set(std::string{name}, JsonToBasicValue(name, value));
        }
        op.WhereString("key", RequireArg(request, "key"));
        for (const auto [name, value] : Items(expect)) {
            if (!IsKnownOptionalField(name)) {
                BadRequest(
                    fmt::format(
                        "field '{}' is not allowed in 'expect' (allowed: bln, "
                        "i32, i64, flt, dbl)",
                        name
                    )
                );
            }
            op.If(std::string{name}, JsonToBasicValue(name, value));
        }
        const auto result = session_->GetTable(std::string{kBasicTable}).ExecuteLwt(op);
        auto response = JsonObject();
        response["applied"] = result.applied;
        if (!result.applied) {
            request.SetResponseStatus(server::http::HttpStatus::kConflict);
            response["current"] = RowToJson(result.previous);
        }
        return formats::json::ToString(response.ExtractValue());
    }

private:
    SessionPtr session_;
};

class DeleteIfExistsHandler final : public server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-kv-delete-if-exists";

    DeleteIfExistsHandler(const components::ComponentConfig& config, const components::ComponentContext& context)
        : HttpHandlerBase(config, context),
          session_(context.FindComponent<components::Scylla>(kComponentName).GetSession()) {}

    std::string HandleRequest(server::http::HttpRequest& request, server::request::RequestContext&) const override {
        request.GetHttpResponse().SetContentType(http::content_type::kApplicationJson);
        operations::DeleteOne op;
        op.WhereString("key", RequireArg(request, "key"));
        op.IfExists();
        const auto result = session_->GetTable(std::string{kBasicTable}).ExecuteLwt(op);
        auto response = JsonObject();
        response["applied"] = result.applied;
        if (!result.applied) {
            request.SetResponseStatus(server::http::HttpStatus::kNotFound);
        }
        return formats::json::ToString(response.ExtractValue());
    }

private:
    SessionPtr session_;
};

class TruncateHandler final : public server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-kv-truncate";

    TruncateHandler(const components::ComponentConfig& config, const components::ComponentContext& context)
        : HttpHandlerBase(config, context),
          session_(context.FindComponent<components::Scylla>(kComponentName).GetSession()) {}

    std::string HandleRequest(server::http::HttpRequest& request, server::request::RequestContext&) const override {
        request.GetHttpResponse().SetContentType(http::content_type::kApplicationJson);
        session_->GetTable(std::string{kBasicTable}).Execute(operations::Truncate{});
        return R"({"status":"ok"})";
    }

private:
    SessionPtr session_;
};

class SchemaInitHandler final : public server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-schema-init";

    SchemaInitHandler(const components::ComponentConfig& config, const components::ComponentContext& context)
        : HttpHandlerBase(config, context),
          session_(context.FindComponent<components::Scylla>(kComponentName).GetSession()) {}

    std::string HandleRequest(server::http::HttpRequest& request, server::request::RequestContext&) const override {
        request.GetHttpResponse().SetContentType(http::content_type::kApplicationJson);

        session_->ExecuteVoid(
            "CREATE TABLE IF NOT EXISTS examples.basic ("
            "key text PRIMARY KEY, "
            "bln boolean, "
            "i32 int, "
            "i64 bigint, "
            "flt float, "
            "dbl double)"
        );

        session_->ExecuteVoid(
            "CREATE TABLE IF NOT EXISTS examples.events ("
            "id uuid PRIMARY KEY, "
            "name text, "
            "created_at timestamp, "
            "source_ip inet, "
            "tags set<text>, "
            "metadata map<text, text>, "
            "scores list<int>)"
        );

        return R"({"status":"ok","tables":["basic","events"]})";
    }

private:
    SessionPtr session_;
};

class RawQueryHandler final : public server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-raw";

    RawQueryHandler(const components::ComponentConfig& config, const components::ComponentContext& context)
        : HttpHandlerBase(config, context),
          session_(context.FindComponent<components::Scylla>(kComponentName).GetSession()) {}

    std::string HandleRequest(server::http::HttpRequest& request, server::request::RequestContext&) const override {
        request.GetHttpResponse().SetContentType(http::content_type::kApplicationJson);
        const auto body = ParseBody(request);
        if (!body.HasMember("query")) {
            BadRequest("JSON field 'query' is required");
        }

        const auto query = body["query"].As<std::string>();

        std::vector<Value> params;
        if (body.HasMember("params")) {
            const auto& p = body["params"];
            for (std::size_t i = 0; i < p.GetSize(); ++i) {
                const auto& item = p[i];
                if (item.IsString()) {
                    params.emplace_back(item.As<std::string>());
                } else if (item.IsBool()) {
                    params.emplace_back(item.As<bool>());
                } else {
                    params.emplace_back(item.As<std::int64_t>());
                }
            }
        }

        if (body.HasMember("void") && body["void"].As<bool>()) {
            session_->ExecuteVoid(query, std::move(params));
            return R"({"status":"ok"})";
        }

        auto rows = session_->Execute(query, std::move(params));
        auto response = JsonObject();
        response["rows"] = RowsToJson(rows);
        response["count"] = static_cast<std::int64_t>(rows.size());
        return formats::json::ToString(response.ExtractValue());
    }

private:
    SessionPtr session_;
};

class EventHandler final : public server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-events";

    EventHandler(const components::ComponentConfig& config, const components::ComponentContext& context)
        : HttpHandlerBase(config, context),
          session_(context.FindComponent<components::Scylla>(kComponentName).GetSession()) {}

    std::string HandleRequest(server::http::HttpRequest& request, server::request::RequestContext&) const override {
        request.GetHttpResponse().SetContentType(http::content_type::kApplicationJson);
        switch (request.GetMethod()) {
            case server::http::HttpMethod::kGet:
                return Get(request);
            case server::http::HttpMethod::kPost:
                return Create(request);
            default:
                BadRequest("Supported methods: GET, POST");
        }
    }

private:
    std::string Get(server::http::HttpRequest& request) const {
        auto id = Uuid::FromString(RequireArg(request, "id"));

        operations::SelectOne op;
        op.AddAllColumns();
        op.WhereUuid("id", id);

        auto row = session_->GetTable(std::string{kEventsTable}).Execute(op);
        if (row.Empty()) {
            request.SetResponseStatus(server::http::HttpStatus::kNotFound);
            return R"({"error":"not_found"})";
        }

        auto event = row.As<Event>();
        return formats::json::ToString(EventToJson(event).ExtractValue());
    }

    std::string Create(const server::http::HttpRequest& request) const {
        const auto body = ParseBody(request);
        if (!body.HasMember("name")) {
            BadRequest("JSON field 'name' is required");
        }

        auto id = Uuid::Random();
        auto now = std::chrono::system_clock::now();

        operations::InsertOne op;
        op.BindUuid("id", id);
        op.BindString("name", body["name"].As<std::string>());
        op.BindTimestamp("created_at", now);

        if (body.HasMember("source_ip")) {
            op.BindInet("source_ip", Inet{body["source_ip"].As<std::string>()});
        }

        if (body.HasMember("tags") && body["tags"].IsArray()) {
            Set tags;
            for (std::size_t i = 0; i < body["tags"].GetSize(); ++i) {
                tags.items.emplace_back(body["tags"][i].As<std::string>());
            }
            op.BindSet("tags", std::move(tags));
        }

        if (body.HasMember("metadata") && body["metadata"].IsObject()) {
            Map metadata;
            for (const auto [k, v] : Items(body["metadata"])) {
                metadata.entries.emplace_back(Value{std::string{k}}, Value{v.As<std::string>()});
            }
            op.BindMap("metadata", std::move(metadata));
        }

        if (body.HasMember("scores") && body["scores"].IsArray()) {
            List scores;
            for (std::size_t i = 0; i < body["scores"].GetSize(); ++i) {
                scores.items.emplace_back(body["scores"][i].As<std::int32_t>());
            }
            op.BindList("scores", std::move(scores));
        }

        if (body.HasMember("ttl")) {
            op.UsingTtl(body["ttl"].As<std::int32_t>());
        }
        if (body.HasMember("write_timestamp")) {
            op.UsingTimestamp(body["write_timestamp"].As<std::int64_t>());
        }

        session_->GetTable(std::string{kEventsTable}).Execute(op);

        auto response = JsonObject();
        response["status"] = std::string{"ok"};
        response["id"] = id.ToString();
        response["created_at"] = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
        return formats::json::ToString(response.ExtractValue());
    }

    SessionPtr session_;
};

class EventListHandler final : public server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-event-list";

    EventListHandler(const components::ComponentConfig& config, const components::ComponentContext& context)
        : HttpHandlerBase(config, context),
          session_(context.FindComponent<components::Scylla>(kComponentName).GetSession()) {}

    std::string HandleRequest(server::http::HttpRequest& request, server::request::RequestContext&) const override {
        request.GetHttpResponse().SetContentType(http::content_type::kApplicationJson);

        std::size_t page_size = 100;
        if (request.HasArg("page_size")) {
            page_size = std::stoul(request.GetArg("page_size"));
        }

        auto cursor = session_->NewCursor("SELECT * FROM examples.events", {}, page_size);

        Rows all_rows;
        while (!cursor.Done()) {
            auto page = cursor.NextPage();
            if (page) {
                all_rows.insert(
                    all_rows.end(),
                    std::make_move_iterator(page->begin()),
                    std::make_move_iterator(page->end())
                );
            }
        }

        auto response = JsonObject();
        response["items"] = RowsToJson(all_rows);
        response["count"] = static_cast<std::int64_t>(all_rows.size());
        return formats::json::ToString(response.ExtractValue());
    }

private:
    SessionPtr session_;
};

}  // namespace
}  // namespace samples::scylladb

int main(int argc, char* argv[]) {
    const auto component_list =
        components::MinimalServerComponentList()
            .Append<clients::dns::Component>()
            .Append<components::Secdist>()
            .Append<components::DefaultSecdistProvider>()
            .Append<components::Scylla>("scylla-db")
            .Append<samples::scylladb::KeyValueHandler>()
            .Append<samples::scylladb::ListHandler>()
            .Append<samples::scylladb::CountHandler>()
            .Append<samples::scylladb::BulkInsertHandler>()
            .Append<samples::scylladb::PaginatedHandler>()
            .Append<samples::scylladb::CreateIfAbsentHandler>()
            .Append<samples::scylladb::CompareAndSetHandler>()
            .Append<samples::scylladb::DeleteIfExistsHandler>()
            .Append<samples::scylladb::TruncateHandler>()
            .Append<samples::scylladb::SchemaInitHandler>()
            .Append<samples::scylladb::RawQueryHandler>()
            .Append<samples::scylladb::EventHandler>()
            .Append<samples::scylladb::EventListHandler>();
    return utils::DaemonMain(argc, argv, component_list);
}
