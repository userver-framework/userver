#include "helpers.hpp"

#include <charconv>
#include <chrono>
#include <cstdint>
#include <iterator>
#include <type_traits>
#include <utility>
#include <vector>

#include <fmt/format.h>

#include <userver/formats/json/exception.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/server/handlers/exceptions.hpp>

namespace samples::scylladb {
namespace {

using Builder = formats::json::ValueBuilder;

Builder JsonArray() { return Builder{formats::common::Type::kArray}; }
Builder JsonObject() { return Builder{formats::common::Type::kObject}; }

template <typename T>
auto JsonCompat(const T& v) {
    if constexpr (std::is_same_v<T, std::int32_t>) {
        return std::int64_t{v};
    } else if constexpr (std::is_same_v<T, float>) {
        return double{v};
    } else {
        return v;
    }
}

template <typename Range, typename Fn>
Builder JsonArrayFrom(const Range& range, Fn&& fn) {
    auto arr = JsonArray();
    for (const auto& item : range) {
        arr.PushBack(fn(item));
    }
    return arr;
}

template <typename Range>
Builder JsonArrayFrom(const Range& range) {
    return JsonArrayFrom(range, [](const auto& x) { return JsonCompat(x); });
}

template <typename KV>
Builder JsonObjectFrom(const KV& items) {
    auto obj = JsonObject();
    for (const auto& [k, v] : items) {
        obj[k] = v;
    }
    return obj;
}

std::int64_t ToMillis(Timestamp ts) {
    return std::chrono::duration_cast<std::chrono::milliseconds>(ts.time_since_epoch()).count();
}

template <typename T>
bool TryJson(const Value& v, Builder& out) {
    if (!v.Is<T>()) {
        return false;
    }
    if constexpr (std::is_same_v<T, Uuid>) {
        out = Builder{v.Get<Uuid>().ToString()};
    } else if constexpr (std::is_same_v<T, Timestamp>) {
        out = Builder{ToMillis(v.Get<Timestamp>())};
    } else {
        out = Builder{JsonCompat(v.Get<T>())};
    }
    return true;
}

template <typename... Ts>
Builder JsonOneOf(const Value& v) {
    Builder out;
    (TryJson<Ts>(v, out) || ...);
    return out;
}

template <typename T, typename Container>
std::vector<T> CollectTyped(const Container& items) {
    std::vector<T> out;
    out.reserve(items.size());
    for (const auto& item : items) {
        if (item.template Is<T>()) {
            out.push_back(item.template Get<T>());
        }
    }
    return out;
}

template <typename T>
std::vector<T> CollectList(const Row& row, std::string_view name) {
    if (const auto* v = row.Find(name); v && v->Is<List>()) {
        return CollectTyped<T>(v->Get<List>().items);
    }
    return {};
}

template <typename T>
std::vector<T> CollectSet(const Row& row, std::string_view name) {
    if (const auto* v = row.Find(name); v && v->Is<Set>()) {
        return CollectTyped<T>(v->Get<Set>().items);
    }
    return {};
}

template <typename K, typename V>
std::vector<std::pair<K, V>> CollectMap(const Row& row, std::string_view name) {
    std::vector<std::pair<K, V>> out;
    if (const auto* v = row.Find(name); v && v->Is<Map>()) {
        for (const auto& [k, val] : v->Get<Map>().entries) {
            if (k.template Is<K>() && val.template Is<V>()) {
                out.emplace_back(k.template Get<K>(), val.template Get<V>());
            }
        }
    }
    return out;
}

}  // namespace

void BadRequest(std::string message) {
    throw server::handlers::ClientError(server::handlers::ExternalBody{std::move(message)});
}

const std::string& RequireArg(const server::http::HttpRequest& request, const std::string_view name) {
    const auto& value = request.GetArg(name);
    if (value.empty()) {
        BadRequest(fmt::format("Query argument '{}' is required", name));
    }
    return value;
}

Value JsonToBasicValue(std::string_view name, const formats::json::Value& json) {
    if (name == "bln") {
        return Value{json.As<bool>()};
    }
    if (name == "i32") {
        return Value{json.As<std::int32_t>()};
    }
    if (name == "i64") {
        return Value{json.As<std::int64_t>()};
    }
    if (name == "flt") {
        return Value{json.As<float>()};
    }
    if (name == "dbl") {
        return Value{json.As<double>()};
    }
    return Value{json.As<std::string>()};
}

formats::json::Value ParseBody(const server::http::HttpRequest& request) {
    try {
        return formats::json::FromString(request.RequestBody());
    } catch (const formats::json::Exception& ex) {
        BadRequest(fmt::format("Invalid JSON body: {}", ex.what()));
    }
}

formats::json::ValueBuilder ValueToJson(const Value& v) {
    if (v.IsNull()) {
        return {};
    }
    return JsonOneOf<std::string, bool, std::int32_t, std::int64_t, double, float, Uuid, Timestamp>(v);
}

formats::json::ValueBuilder RowToJson(const Row& row) {
    auto obj = JsonObject();
    for (const auto& [name, value] : row) {
        if (!value.IsNull()) {
            obj[name] = ValueToJson(value);
        }
    }
    return obj;
}

formats::json::ValueBuilder RowsToJson(const Rows& rows) {
    return JsonArrayFrom(rows, [](const Row& r) { return RowToJson(r); });
}

std::string ToHex(std::string_view data) {
    std::string out;
    out.reserve(data.size() * 2);
    for (unsigned char c : data) {
        fmt::format_to(std::back_inserter(out), "{:02x}", c);
    }
    return out;
}

std::string FromHex(std::string_view hex) {
    std::string out;
    out.reserve(hex.size() / 2);
    for (std::size_t i = 0; i + 1 < hex.size(); i += 2) {
        unsigned byte = 0;
        std::from_chars(hex.data() + i, hex.data() + i + 2, byte, 16);
        out.push_back(static_cast<char>(byte));
    }
    return out;
}

void DecodeRow(const Row& row, Event& out) {
    out.id = row.Get<Uuid>("id");
    out.name = row.Get<std::string>("name");
    out.tags = CollectSet<std::string>(row, "tags");
    out.metadata = CollectMap<std::string, std::string>(row, "metadata");
    out.scores = CollectList<std::int32_t>(row, "scores");
    if (auto ts = row.TryGet<Timestamp>("created_at")) {
        out.created_at = *ts;
    }
    if (auto ip = row.TryGet<Inet>("source_ip")) {
        out.source_ip = ip->text;
    }
}

formats::json::ValueBuilder EventToJson(const Event& event) {
    auto obj = JsonObject();
    obj["id"] = event.id.ToString();
    obj["name"] = event.name;
    obj["created_at"] = ToMillis(event.created_at);
    obj["tags"] = JsonArrayFrom(event.tags);
    obj["scores"] = JsonArrayFrom(event.scores);
    obj["metadata"] = JsonObjectFrom(event.metadata);
    if (!event.source_ip.empty()) {
        obj["source_ip"] = event.source_ip;
    }
    return obj;
}

}  // namespace samples::scylladb
