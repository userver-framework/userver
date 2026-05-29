#include "cql_builder.hpp"

#include <fmt/format.h>

#include <cassandra.h>

#include <userver/storages/scylla/exception.hpp>
#include <userver/tracing/tags.hpp>

#include <storages/scylla/driver/value_encode.hpp>

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

}  // namespace

ValidatedCql MakeValidatedIdentifier(std::string_view name, std::string_view role) {
    if (!IsValidCqlIdentifier(name)) {
        throw InvalidQueryArgumentException(fmt::format(
            "Invalid {}: '{}'. Expected a plain identifier matching "
            "[a-zA-Z][a-zA-Z0-9_]* (up to {} characters). ",
            role,
            name,
            kMaxCqlIdentifierLength
        ));
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
