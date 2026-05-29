#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <fmt/format.h>
#include <fmt/ranges.h>

#include <userver/storages/scylla/exception.hpp>
#include <userver/storages/scylla/value.hpp>

#include <storages/scylla/driver/cass_wrappers.hpp>
#include <storages/scylla/driver/request_context.hpp>
#include <storages/scylla/operations_impl.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::scylla::impl::driver::cql {

struct CqlQuery {
    std::string text;
    std::vector<Value> values;
};

class ValidatedCql {
public:
    std::string_view Str() const noexcept { return value_; }

private:
    friend ValidatedCql MakeValidatedIdentifier(std::string_view, std::string_view);
    friend ValidatedCql MakeValidatedFullTableName(std::string_view, std::string_view);
    explicit ValidatedCql(std::string value) noexcept : value_(std::move(value)) {}
    std::string value_;
};

ValidatedCql MakeValidatedIdentifier(std::string_view name, std::string_view role);

ValidatedCql MakeValidatedFullTableName(std::string_view keyspace, std::string_view table);

CassStatementPtr Prepare(RequestContext& ctx, const CqlQuery& query);

}  // namespace storages::scylla::impl::driver::cql

USERVER_NAMESPACE_END

template <>
struct fmt::formatter<USERVER_NAMESPACE::storages::scylla::impl::driver::cql::ValidatedCql>
    : fmt::formatter<std::string_view> {
    template <typename FormatContext>
    auto format(const USERVER_NAMESPACE::storages::scylla::impl::driver::cql::ValidatedCql& v, FormatContext& ctx)
        const {
        return fmt::formatter<std::string_view>::format(v.Str(), ctx);
    }
};

USERVER_NAMESPACE_BEGIN

namespace storages::scylla::impl::driver::cql {

inline std::string Placeholders(std::size_t n) {
    if (n == 0) {
        return {};
    }
    std::string result = "?";
    for (std::size_t i = 1; i < n; ++i) {
        result += ", ?";
    }
    return result;
}

inline std::string BuildColumnList(const std::vector<std::string>& columns, bool select_all) {
    if (select_all || columns.empty()) {
        return "*";
    }
    std::vector<ValidatedCql> validated;

    validated.reserve(columns.size());

    for (const auto& c : columns) {
        validated.emplace_back(MakeValidatedIdentifier(c, "column name"));
    }
    return fmt::format("{}", fmt::join(validated, ", "));
}

struct WhereFragment {
    std::string text;
    std::vector<Value> values;
};

inline WhereFragment BuildWhereClause(const std::vector<operations::NamedValue>& conditions) {
    WhereFragment out;
    if (conditions.empty()) {
        return out;
    }

    std::vector<std::string> clauses;
    clauses.reserve(conditions.size());
    out.values.reserve(conditions.size());
    for (const auto& c : conditions) {
        clauses.emplace_back(fmt::format("{} = ?", MakeValidatedIdentifier(c.column_name, "column name")));
        out.values.emplace_back(c.value);
    }
    out.text = fmt::format(" WHERE {}", fmt::join(clauses, " AND "));
    return out;
}

struct IfFragment {
    std::string text;
    std::vector<Value> values;
};

inline IfFragment BuildIfClause(const std::vector<operations::NamedValue>& if_conditions, bool if_exists) {
    IfFragment out;
    if (if_exists) {
        out.text = " IF EXISTS";
        return out;
    }
    if (if_conditions.empty()) {
        return out;
    }

    std::vector<std::string> clauses;
    clauses.reserve(if_conditions.size());
    out.values.reserve(if_conditions.size());
    for (const auto& c : if_conditions) {
        clauses.emplace_back(fmt::format("{} = ?", MakeValidatedIdentifier(c.column_name, "column name")));
        out.values.emplace_back(c.value);
    }
    out.text = fmt::format(" IF {}", fmt::join(clauses, " AND "));
    return out;
}

struct UsingFragment {
    std::string text;
    std::vector<Value> values;
};

inline UsingFragment BuildUsingClause(const operations::UsingClause& uc) {
    UsingFragment out;
    std::vector<std::string> parts;
    if (uc.ttl_seconds) {
        parts.emplace_back("TTL ?");
        out.values.emplace_back(Value{*uc.ttl_seconds});
    }
    if (uc.timestamp_micros) {
        parts.emplace_back("TIMESTAMP ?");
        out.values.emplace_back(Value{*uc.timestamp_micros});
    }
    if (parts.empty()) {
        return out;
    }
    out.text = fmt::format(" USING {}", fmt::join(parts, " AND "));
    return out;
}

inline auto Insert(
    const std::vector<operations::NamedValue>& bindings,
    bool if_not_exists,
    const operations::UsingClause& using_clause
) {
    return [&bindings, if_not_exists, &using_clause](const ValidatedCql& full_table) -> CqlQuery {
        if (bindings.empty()) {
            throw QueryException("InsertOne: no bindings provided");
        }

        std::vector<ValidatedCql> names;
        names.reserve(bindings.size());
        std::vector<Value> values;
        values.reserve(bindings.size() + 2);
        for (const auto& b : bindings) {
            names.emplace_back(MakeValidatedIdentifier(b.column_name, "column name"));
            values.emplace_back(b.value);
        }

        auto using_frag = BuildUsingClause(using_clause);
        for (auto& v : using_frag.values) {
            values.emplace_back(std::move(v));
        }

        return {
            fmt::format(
                "INSERT INTO {} ({}) VALUES ({}){}{}",
                full_table,
                fmt::join(names, ", "),
                Placeholders(bindings.size()),
                if_not_exists ? " IF NOT EXISTS" : "",
                using_frag.text
            ),
            std::move(values),
        };
    };
}

inline auto SelectOne(
    const std::vector<std::string>& columns,
    bool select_all,
    const std::vector<operations::NamedValue>& conditions
) {
    return [&columns, select_all, &conditions](const ValidatedCql& full_table) -> CqlQuery {
        auto where = BuildWhereClause(conditions);
        return {
            fmt::format("SELECT {} FROM {}{} LIMIT 1", BuildColumnList(columns, select_all), full_table, where.text),
            std::move(where.values),
        };
    };
}

inline auto SelectMany(
    const std::vector<std::string>& columns,
    bool select_all,
    const std::vector<operations::NamedValue>& conditions,
    std::size_t limit,
    bool allow_filtering
) {
    return [&columns, select_all, &conditions, limit, allow_filtering](const ValidatedCql& full_table) -> CqlQuery {
        auto where = BuildWhereClause(conditions);
        //  limit is size_t and fmt gives us decimal only, so no injection
        std::string limit_text = limit == 0 ? std::string{} : fmt::format(" LIMIT {}", limit);
        std::string af_text = allow_filtering ? " ALLOW FILTERING" : "";
        return {
            fmt::format(
                "SELECT {} FROM {}{}{}{}",
                BuildColumnList(columns, select_all),
                full_table,
                where.text,
                limit_text,
                af_text
            ),
            std::move(where.values),
        };
    };
}

inline auto DeleteRows(const std::vector<operations::NamedValue>& conditions) {
    return [&conditions](const ValidatedCql& full_table) -> CqlQuery {
        if (conditions.empty()) {
            throw QueryException("DeleteOne: WHERE clause is required");
        }
        auto where = BuildWhereClause(conditions);
        return {
            fmt::format("DELETE FROM {}{}", full_table, where.text),
            std::move(where.values),
        };
    };
}

inline auto DeleteRowsLwt(
    const std::vector<operations::NamedValue>& conditions,
    const std::vector<operations::NamedValue>& if_conditions,
    bool if_exists
) {
    return [&conditions, &if_conditions, if_exists](const ValidatedCql& full_table) -> CqlQuery {
        if (conditions.empty()) {
            throw QueryException("DeleteOne: WHERE clause is required");
        }
        auto where = BuildWhereClause(conditions);
        auto if_frag = BuildIfClause(if_conditions, if_exists);

        auto values = std::move(where.values);
        values.reserve(values.size() + if_frag.values.size());
        for (auto& v : if_frag.values) {
            values.emplace_back(std::move(v));
        }

        return {
            fmt::format("DELETE FROM {}{}{}", full_table, where.text, if_frag.text),
            std::move(values),
        };
    };
}

inline auto Update(
    const std::vector<operations::NamedValue>& assignments,
    const std::vector<operations::NamedValue>& conditions,
    const operations::UsingClause& using_clause
) {
    return [&assignments, &conditions, &using_clause](const ValidatedCql& full_table) -> CqlQuery {
        if (assignments.empty()) {
            throw QueryException("UpdateOne: no assignments provided");
        }
        if (conditions.empty()) {
            throw QueryException("UpdateOne: WHERE clause is required");
        }

        auto using_frag = BuildUsingClause(using_clause);

        std::vector<std::string> set_clauses;
        set_clauses.reserve(assignments.size());
        std::vector<Value> values;
        values.reserve(using_frag.values.size() + assignments.size() + conditions.size());

        for (auto& v : using_frag.values) {
            values.emplace_back(std::move(v));
        }
        for (const auto& a : assignments) {
            set_clauses.emplace_back(fmt::format("{} = ?", MakeValidatedIdentifier(a.column_name, "column name")));
            values.emplace_back(a.value);
        }

        auto where = BuildWhereClause(conditions);
        for (auto& v : where.values) {
            values.emplace_back(std::move(v));
        }

        return {
            fmt::format("UPDATE {}{} SET {}{}", full_table, using_frag.text, fmt::join(set_clauses, ", "), where.text),
            std::move(values),
        };
    };
}

inline auto UpdateLwt(
    const std::vector<operations::NamedValue>& assignments,
    const std::vector<operations::NamedValue>& conditions,
    const std::vector<operations::NamedValue>& if_conditions,
    bool if_exists,
    const operations::UsingClause& using_clause
) {
    return [&assignments, &conditions, &if_conditions, if_exists, &using_clause](const ValidatedCql& full_table
           ) -> CqlQuery {
        if (assignments.empty()) {
            throw QueryException("UpdateOne: no assignments provided");
        }
        if (conditions.empty()) {
            throw QueryException("UpdateOne: WHERE clause is required");
        }

        auto using_frag = BuildUsingClause(using_clause);

        std::vector<std::string> set_clauses;
        set_clauses.reserve(assignments.size());
        std::vector<Value> values;
        values.reserve(using_frag.values.size() + assignments.size() + conditions.size() + if_conditions.size());

        for (auto& v : using_frag.values) {
            values.emplace_back(std::move(v));
        }
        for (const auto& a : assignments) {
            set_clauses.emplace_back(fmt::format("{} = ?", MakeValidatedIdentifier(a.column_name, "column name")));
            values.emplace_back(a.value);
        }

        auto where = BuildWhereClause(conditions);
        for (auto& v : where.values) {
            values.emplace_back(std::move(v));
        }

        auto if_frag = BuildIfClause(if_conditions, if_exists);
        for (auto& v : if_frag.values) {
            values.emplace_back(std::move(v));
        }

        return {
            fmt::format(
                "UPDATE {}{} SET {}{}{}",
                full_table,
                using_frag.text,
                fmt::join(set_clauses, ", "),
                where.text,
                if_frag.text
            ),
            std::move(values),
        };
    };
}

inline auto Count(const std::vector<operations::NamedValue>& conditions) {
    return [&conditions](const ValidatedCql& full_table) -> CqlQuery {
        auto where = BuildWhereClause(conditions);
        return {
            fmt::format("SELECT COUNT(*) FROM {}{}", full_table, where.text),
            std::move(where.values),
        };
    };
}

CassStatementPtr BuildDropKeyspaceStatement(const RequestContext& ctx);
CassStatementPtr BuildTruncateStatement(const RequestContext& ctx);

}  // namespace storages::scylla::impl::driver::cql

USERVER_NAMESPACE_END
