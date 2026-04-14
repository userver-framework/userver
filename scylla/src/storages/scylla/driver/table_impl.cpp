#include "table_impl.hpp"

#include <cstdint>
#include <optional>
#include <utility>

#include <cassandra.h>

#include <userver/storages/scylla/exception.hpp>

#include <storages/scylla/driver/cass_wrappers.hpp>
#include <storages/scylla/driver/cql_builder.hpp>
#include <storages/scylla/driver/request_context.hpp>
#include <storages/scylla/driver/session_impl.hpp>
#include <storages/scylla/operations_impl.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::scylla::impl::driver {

namespace {

DriverSessionImpl& GetDriverSession(const SessionImplPtr& session_impl) {
    auto* driver = dynamic_cast<DriverSessionImpl*>(session_impl.get());
    if (!driver) {
        throw ScyllaException() << "Session is not a DriverSessionImpl";
    }
    return *driver;
}

std::optional<operations::SelectOne::Value> ExtractColumnValue(const CassValue* cass_val) {
    switch (cass_value_type(cass_val)) {
        case CASS_VALUE_TYPE_VARCHAR:
        case CASS_VALUE_TYPE_TEXT: {
            const char* str = nullptr;
            std::size_t str_length = 0;
            cass_value_get_string(cass_val, &str, &str_length);
            return std::string(str, str_length);
        }
        case CASS_VALUE_TYPE_INT: {
            int32_t v = 0;
            cass_value_get_int32(cass_val, &v);
            return v;
        }
        case CASS_VALUE_TYPE_BIGINT: {
            int64_t v = 0;
            cass_value_get_int64(cass_val, &v);
            return v;
        }
        case CASS_VALUE_TYPE_BOOLEAN: {
            cass_bool_t v = cass_false;
            cass_value_get_bool(cass_val, &v);
            return static_cast<bool>(v);
        }
        case CASS_VALUE_TYPE_FLOAT: {
            float v = 0.0f;
            cass_value_get_float(cass_val, &v);
            return v;
        }
        case CASS_VALUE_TYPE_DOUBLE: {
            double v = 0.0;
            cass_value_get_double(cass_val, &v);
            return v;
        }
        default:
            return std::nullopt;
    }
}

template <typename Row>
Row ExtractRowImpl(const CassResult* result, const CassRow* cass_row) {
    Row row;
    if (!cass_row) return row;

    const std::size_t col_count = cass_result_column_count(result);
    row.reserve(col_count);
    for (std::size_t i = 0; i < col_count; ++i) {
        const char* col_name = nullptr;
        std::size_t col_name_length = 0;
        cass_result_column_name(result, i, &col_name, &col_name_length);

        auto value = ExtractColumnValue(cass_row_get_column(cass_row, i));
        if (!value) continue;

        row.emplace_back(std::string(col_name, col_name_length), std::move(*value));
    }
    return row;
}

operations::SelectOne::Row ExtractFirstRow(const CassResult* result) {
    return ExtractRowImpl<operations::SelectOne::Row>(result, cass_result_first_row(result));
}

operations::SelectMany::ResultSet ExtractAllRows(const CassResult* result) {
    operations::SelectMany::ResultSet rows;
    rows.reserve(cass_result_row_count(result));

    CassIteratorPtr iterator{cass_iterator_from_result(result)};
    while (cass_iterator_next(iterator.get())) {
        rows.push_back(
            ExtractRowImpl<operations::SelectMany::Row>(result, cass_iterator_get_row(iterator.get())));
    }
    return rows;
}

int64_t ExtractCount(const CassResult* result) {
    const CassRow* row = cass_result_first_row(result);
    if (!row) {
        throw ScyllaException() << "Count: empty result";
    }
    const CassValue* value = cass_row_get_column(row, 0);
    if (!value) {
        throw ScyllaException() << "Count: no column in result row";
    }
    int64_t count = 0;
    if (cass_value_get_int64(value, &count) != CASS_OK) {
        throw ScyllaException() << "Count: result column is not a bigint";
    }
    return count;
}

template <typename QueryBuilder, typename Parse>
auto RunTableRequest(
    DriverSessionImpl& session,
    std::string_view span_name,
    const std::string& keyspace,
    std::string_view table,
    std::string_view action,
    bool idempotent,
    QueryBuilder&& build_query,
    Parse&& parse
) {
    auto ctx = MakeTableRequestContext(session, std::string{span_name}, keyspace, table);
    auto full_table = cql::BuildFullTableName(ctx.keyspace, ctx.table);
    auto stmt = cql::Prepare(ctx, std::forward<QueryBuilder>(build_query)(full_table));
    auto result = ExecuteStatement(ctx, std::move(stmt), idempotent, action);
    return std::forward<Parse>(parse)(result.get());
}

constexpr auto kNoParse = [](const CassResult*) noexcept {};

}

DriverTableImpl::DriverTableImpl(SessionImplPtr session_impl, std::string keyspace_name, std::string table_name)
    : TableImpl(std::move(keyspace_name), std::move(table_name)), session_impl_(std::move(session_impl)) {}

void DriverTableImpl::Execute(const operations::InsertOne& op) {
    RunTableRequest(
        GetDriverSession(session_impl_),
        "scylla_insert_one", GetKeyspaceName(), GetTableName(),
        "InsertOne", true,
        cql::Insert(op.impl_->bindings),
        kNoParse);
}

operations::SelectOne::Row DriverTableImpl::Execute(const operations::SelectOne& op) {
    return RunTableRequest(
        GetDriverSession(session_impl_),
        "scylla_select_one", GetKeyspaceName(), GetTableName(),
        "SelectOne", true,
        cql::SelectOne(op.impl_->columns, op.impl_->select_all, op.impl_->conditions),
        ExtractFirstRow);
}

operations::SelectMany::ResultSet DriverTableImpl::Execute(const operations::SelectMany& op) {
    return RunTableRequest(
        GetDriverSession(session_impl_),
        "scylla_select_many", GetKeyspaceName(), GetTableName(),
        "SelectMany", true,
        cql::SelectMany(op.impl_->columns, op.impl_->select_all, op.impl_->conditions, op.impl_->limit),
        ExtractAllRows);
}

void DriverTableImpl::Execute(const operations::DeleteOne& op) {

    RunTableRequest(
        GetDriverSession(session_impl_),
        "scylla_delete_one", GetKeyspaceName(), GetTableName(),
        "DeleteOne", true,
        cql::DeleteRows(op.impl_->conditions),
        kNoParse);
}

void DriverTableImpl::Execute(const operations::UpdateOne& op) {

    RunTableRequest(
        GetDriverSession(session_impl_),
        "scylla_update_one", GetKeyspaceName(), GetTableName(),
        "UpdateOne", true,
        cql::Update(op.impl_->assignments, op.impl_->conditions),
        kNoParse);
}

int64_t DriverTableImpl::Execute(const operations::Count& op) {
    return RunTableRequest(
        GetDriverSession(session_impl_),
        "scylla_count", GetKeyspaceName(), GetTableName(),
        "Count", true,
        cql::Count(op.impl_->conditions),
        ExtractCount);
}

void DriverTableImpl::Execute(const operations::InsertMany& op) {
    const auto& rows = op.impl_->rows;

    if (rows.empty() || rows.front().empty()) {
        throw QueryException("InsertMany: no rows provided");
    }

    const auto& first_row = rows.front();
    for (std::size_t r = 1; r < rows.size(); ++r) {
        const auto& row = rows[r];
        if (row.empty()) {
            throw QueryException("InsertMany: empty row in batch");
        }
        if (row.size() != first_row.size()) {
            throw QueryException("InsertMany: rows have inconsistent column counts");
        }
        for (std::size_t c = 0; c < row.size(); ++c) {
            if (row[c].column_name != first_row[c].column_name) {
                throw QueryException("InsertMany: rows have inconsistent column names");
            }
        }
    }

    auto& session = GetDriverSession(session_impl_);
    auto ctx = MakeTableRequestContext(
        session, "scylla_insert_many", GetKeyspaceName(), GetTableName());
    const auto full_table = cql::BuildFullTableName(ctx.keyspace, ctx.table);

    CassBatchPtr batch{cass_batch_new(CASS_BATCH_TYPE_LOGGED)};
    for (const auto& row : rows) {

        auto stmt = cql::Prepare(ctx, cql::Insert(row)(full_table));
        if (cass_batch_add_statement(batch.get(), stmt.get()) != CASS_OK) {
            throw QueryException("InsertMany: failed to add statement to batch");
        }
    }

    ExecuteBatch(ctx, std::move(batch), true, "InsertMany");
}

void DriverTableImpl::Execute(const operations::Truncate& ) {
    auto& session = GetDriverSession(session_impl_);
    auto ctx = MakeTableRequestContext(
        session, "scylla_truncate", GetKeyspaceName(), GetTableName());

    ExecuteStatement(ctx, cql::BuildTruncateStatement(ctx), false, "Truncate");
}

}

USERVER_NAMESPACE_END
