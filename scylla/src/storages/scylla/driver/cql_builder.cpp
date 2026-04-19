#include "cql_builder.hpp"

#include <chrono>
#include <cstring>
#include <memory>

#include <fmt/format.h>

#include <cassandra.h>

#include <userver/storages/scylla/exception.hpp>
#include <userver/tracing/tags.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::scylla::impl::driver::cql {

namespace {

constexpr std::size_t kMaxCqlIdentifierLength = 48;

constexpr bool IsAsciiAlpha(char c) noexcept { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }

constexpr bool IsAsciiDigit(char c) noexcept { return c >= '0' && c <= '9'; }

bool IsValidCqlIdentifier(std::string_view name) noexcept {
    if (name.empty() || name.size() > kMaxCqlIdentifierLength) {
        return false;
    }
    if (!IsAsciiAlpha(name.front())) {
        return false;
    }
    for (const char c : name) {
        if (!IsAsciiAlpha(c) && !IsAsciiDigit(c) && c != '_') {
            return false;
        }
    }
    return true;
}

CassUuid ToCassUuid(const Uuid& u) {
    CassUuid out;
    out.time_and_version = u.time_and_version;
    out.clock_seq_and_node = u.clock_seq_and_node;
    return out;
}

void CheckRc(CassError rc, std::string_view what) {
    if (rc != CASS_OK) {
        throw QueryException(fmt::format("{}: {}", what, cass_error_desc(rc)));
    }
}

void AppendValueToCollection(CassCollection* col, const Value& v);

CassCollectionPtr BuildList(const List& list) {
    CassCollectionPtr col{cass_collection_new(CASS_COLLECTION_TYPE_LIST, list.items.size())};
    for (const auto& item : list.items) {
        AppendValueToCollection(col.get(), item);
    }
    return col;
}

CassCollectionPtr BuildSet(const Set& set) {
    CassCollectionPtr col{cass_collection_new(CASS_COLLECTION_TYPE_SET, set.items.size())};
    for (const auto& item : set.items) {
        AppendValueToCollection(col.get(), item);
    }
    return col;
}

CassCollectionPtr BuildMap(const Map& map) {
    CassCollectionPtr col{cass_collection_new(CASS_COLLECTION_TYPE_MAP, map.entries.size())};
    for (const auto& [k, v] : map.entries) {
        AppendValueToCollection(col.get(), k);
        AppendValueToCollection(col.get(), v);
    }
    return col;
}

void AppendValueToCollection(CassCollection* col, const Value& v) {
    std::visit(
        [col](const auto& alt) {
            using T = std::decay_t<decltype(alt)>;
            if constexpr (std::is_same_v<T, std::monostate>) {
                throw QueryException("Cannot append NULL inside a CQL collection");
            } else if constexpr (std::is_same_v<T, std::string>) {
                CheckRc(cass_collection_append_string_n(col, alt.data(), alt.size()), "collection.append_string");
            } else if constexpr (std::is_same_v<T, std::int8_t>) {
                CheckRc(cass_collection_append_int8(col, alt), "collection.append_int8");
            } else if constexpr (std::is_same_v<T, std::int16_t>) {
                CheckRc(cass_collection_append_int16(col, alt), "collection.append_int16");
            } else if constexpr (std::is_same_v<T, std::int32_t>) {
                CheckRc(cass_collection_append_int32(col, alt), "collection.append_int32");
            } else if constexpr (std::is_same_v<T, std::int64_t>) {
                CheckRc(cass_collection_append_int64(col, alt), "collection.append_int64");
            } else if constexpr (std::is_same_v<T, bool>) {
                CheckRc(cass_collection_append_bool(col, alt ? cass_true : cass_false), "collection.append_bool");
            } else if constexpr (std::is_same_v<T, float>) {
                CheckRc(cass_collection_append_float(col, alt), "collection.append_float");
            } else if constexpr (std::is_same_v<T, double>) {
                CheckRc(cass_collection_append_double(col, alt), "collection.append_double");
            } else if constexpr (std::is_same_v<T, Uuid>) {
                CheckRc(cass_collection_append_uuid(col, ToCassUuid(alt)), "collection.append_uuid");
            } else if constexpr (std::is_same_v<T, Timestamp>) {
                const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(alt.time_since_epoch()).count();
                CheckRc(
                    cass_collection_append_int64(col, static_cast<cass_int64_t>(ms)),
                    "collection.append_timestamp"
                );
            } else if constexpr (std::is_same_v<T, Blob>) {
                CheckRc(
                    cass_collection_append_bytes(col, reinterpret_cast<const cass_byte_t*>(alt.data()), alt.size()),
                    "collection.append_bytes"
                );
            } else if constexpr (std::is_same_v<T, Inet>) {
                CassInet inet;
                if (cass_inet_from_string(alt.text.c_str(), &inet) != CASS_OK) {
                    throw InvalidQueryArgumentException(fmt::format("Invalid inet '{}'", alt.text));
                }
                CheckRc(cass_collection_append_inet(col, inet), "collection.append_inet");
            } else {
                throw QueryException("Nested collections inside collections are not supported");
            }
        },
        v.Raw()
    );
}

}  // namespace

void BindValue(CassStatement* stmt, std::size_t i, const Value& v) {
    std::visit(
        [stmt, i](const auto& alt) {
            using T = std::decay_t<decltype(alt)>;
            if constexpr (std::is_same_v<T, std::monostate>) {
                CheckRc(cass_statement_bind_null(stmt, i), "bind_null");
            } else if constexpr (std::is_same_v<T, std::string>) {
                CheckRc(cass_statement_bind_string_n(stmt, i, alt.data(), alt.size()), "bind_string");
            } else if constexpr (std::is_same_v<T, std::int8_t>) {
                CheckRc(cass_statement_bind_int8(stmt, i, alt), "bind_int8");
            } else if constexpr (std::is_same_v<T, std::int16_t>) {
                CheckRc(cass_statement_bind_int16(stmt, i, alt), "bind_int16");
            } else if constexpr (std::is_same_v<T, std::int32_t>) {
                CheckRc(cass_statement_bind_int32(stmt, i, alt), "bind_int32");
            } else if constexpr (std::is_same_v<T, std::int64_t>) {
                CheckRc(cass_statement_bind_int64(stmt, i, alt), "bind_int64");
            } else if constexpr (std::is_same_v<T, bool>) {
                CheckRc(cass_statement_bind_bool(stmt, i, alt ? cass_true : cass_false), "bind_bool");
            } else if constexpr (std::is_same_v<T, float>) {
                CheckRc(cass_statement_bind_float(stmt, i, alt), "bind_float");
            } else if constexpr (std::is_same_v<T, double>) {
                CheckRc(cass_statement_bind_double(stmt, i, alt), "bind_double");
            } else if constexpr (std::is_same_v<T, Uuid>) {
                CheckRc(cass_statement_bind_uuid(stmt, i, ToCassUuid(alt)), "bind_uuid");
            } else if constexpr (std::is_same_v<T, Timestamp>) {
                const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(alt.time_since_epoch()).count();
                CheckRc(cass_statement_bind_int64(stmt, i, static_cast<cass_int64_t>(ms)), "bind_timestamp");
            } else if constexpr (std::is_same_v<T, Date>) {
                CheckRc(cass_statement_bind_uint32(stmt, i, alt.days_since_epoch), "bind_date");
            } else if constexpr (std::is_same_v<T, Time>) {
                CheckRc(
                    cass_statement_bind_int64(stmt, i, static_cast<cass_int64_t>(alt.nanoseconds_of_day)),
                    "bind_time"
                );
            } else if constexpr (std::is_same_v<T, Blob>) {
                CheckRc(
                    cass_statement_bind_bytes(stmt, i, reinterpret_cast<const cass_byte_t*>(alt.data()), alt.size()),
                    "bind_bytes"
                );
            } else if constexpr (std::is_same_v<T, Inet>) {
                CassInet inet;
                if (cass_inet_from_string(alt.text.c_str(), &inet) != CASS_OK) {
                    throw InvalidQueryArgumentException(fmt::format("Invalid inet '{}'", alt.text));
                }
                CheckRc(cass_statement_bind_inet(stmt, i, inet), "bind_inet");
            } else if constexpr (std::is_same_v<T, std::shared_ptr<List>>) {
                auto col = BuildList(*alt);
                CheckRc(cass_statement_bind_collection(stmt, i, col.get()), "bind_list");
            } else if constexpr (std::is_same_v<T, std::shared_ptr<Set>>) {
                auto col = BuildSet(*alt);
                CheckRc(cass_statement_bind_collection(stmt, i, col.get()), "bind_set");
            } else if constexpr (std::is_same_v<T, std::shared_ptr<Map>>) {
                auto col = BuildMap(*alt);
                CheckRc(cass_statement_bind_collection(stmt, i, col.get()), "bind_map");
            } else {
                static_assert(!std::is_same_v<T, T>, "Unhandled Value alternative");
            }
        },
        v.Raw()
    );
}

ValidatedCql MakeValidatedIdentifier(std::string_view name, std::string_view role) {
    if (!IsValidCqlIdentifier(name)) {
        throw InvalidQueryArgumentException(fmt::format(
            "Invalid {}: '{}'. Expected a plain identifier matching "
            "[a-zA-Z][a-zA-Z0-9_]* (up to {} characters). "
            "For quoted or schema-qualified names, use Session::Execute with a raw query.",
            role, name, kMaxCqlIdentifierLength));
    }
    return ValidatedCql{std::string{name}};
}

ValidatedCql MakeValidatedFullTableName(std::string_view keyspace, std::string_view table) {
    auto validated_table = MakeValidatedIdentifier(table, "table name");
    if (keyspace.empty()) {
        return validated_table;
    }
    auto validated_keyspace = MakeValidatedIdentifier(keyspace, "keyspace name");
    return ValidatedCql{fmt::format("{}.{}", validated_keyspace, validated_table)};
}

CassStatementPtr Prepare(RequestContext& ctx, const CqlQuery& query) {
    ctx.span.AddTag(tracing::kDatabaseStatement, query.text);
    auto stmt = ctx.prepared_cache.GetOrPrepare(ctx.native_session, query.text);
    for (std::size_t i = 0; i < query.values.size(); ++i) {
        BindValue(stmt.get(), i, query.values[i]);
    }
    return stmt;
}

CassStatementPtr BuildDropKeyspaceStatement(const RequestContext& ctx) {
    if (ctx.keyspace.empty()) {
        throw InvalidConfigException("Cannot drop keyspace: no keyspace name provided");
    }
    const auto keyspace = MakeValidatedIdentifier(ctx.keyspace, "keyspace name");
    return CassStatementPtr{cass_statement_new(fmt::format("DROP KEYSPACE IF EXISTS {}", keyspace).c_str(), 0)};
}

CassStatementPtr BuildTruncateStatement(const RequestContext& ctx) {
    if (ctx.table.empty()) {
        throw InvalidConfigException("Cannot truncate: no table name provided");
    }
    const auto full_table = MakeValidatedFullTableName(ctx.keyspace, ctx.table);
    return CassStatementPtr{cass_statement_new(fmt::format("TRUNCATE {}", full_table).c_str(), 0)};
}

}  // namespace storages::scylla::impl::driver::cql

USERVER_NAMESPACE_END
