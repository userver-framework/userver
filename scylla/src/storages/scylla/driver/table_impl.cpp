#include "table_impl.hpp"

#include <cstdint>
#include <utility>

#include <cassandra.h>

#include <userver/storages/scylla/exception.hpp>

#include <storages/scylla/driver/cass_wrappers.hpp>
#include <storages/scylla/driver/cql_builder.hpp>
#include <storages/scylla/driver/request_context.hpp>
#include <storages/scylla/driver/session_impl.hpp>
#include <storages/scylla/driver/value_extract.hpp>
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

Row ExtractFirstRow(const CassResult* result) { return ExtractRow(result, cass_result_first_row(result)); }

std::int64_t ExtractCount(const CassResult* result) {
    const CassRow* row = cass_result_first_row(result);
    if (!row) {
        throw ScyllaException() << "Count: empty result";
    }
    const CassValue* value = cass_row_get_column(row, 0);
    if (!value) {
        throw ScyllaException() << "Count: no column in result row";
    }
    cass_int64_t count = 0;
    if (cass_value_get_int64(value, &count) != CASS_OK) {
        throw ScyllaException() << "Count: result column is not a bigint";
    }
    return static_cast<std::int64_t>(count);
}

template <typename QueryBuilder, typename Parse>
auto RunTableRequest(
    DriverSessionImpl& session,
    std::string_view span_name,
    std::string_view keyspace,
    std::string_view table,
    std::string_view action,
    bool idempotent,
    QueryBuilder&& build_query,
    Parse&& parse
) {
    auto ctx = MakeTableRequestContext(session, std::string{span_name}, keyspace, table);
    auto full_table = cql::MakeValidatedFullTableName(ctx.keyspace, ctx.table);
    auto stmt = cql::Prepare(ctx, std::forward<QueryBuilder>(build_query)(full_table));
    auto result = ExecuteStatement(ctx, std::move(stmt), idempotent, action);
    return std::forward<Parse>(parse)(result.get());
}

constexpr auto kNoParse = [](const CassResult*) noexcept {};

}  // namespace

DriverTableImpl::DriverTableImpl(SessionImplPtr session_impl, std::string keyspace_name, std::string table_name)
    : TableImpl(std::move(keyspace_name), std::move(table_name)),
      session_impl_(std::move(session_impl))
{}

void DriverTableImpl::Execute(const operations::InsertOne& op) {
    RunTableRequest(
        GetDriverSession(session_impl_),
        "scylla_insert_one",
        GetKeyspaceName(),
        GetTableName(),
        "InsertOne",
        true,
        cql::Insert(op.impl_->bindings, /*if_not_exists=*/false, op.impl_->using_clause),
        kNoParse
    );
}

Row DriverTableImpl::Execute(const operations::SelectOne& op) {
    return RunTableRequest(
        GetDriverSession(session_impl_),
        "scylla_select_one",
        GetKeyspaceName(),
        GetTableName(),
        "SelectOne",
        true,
        cql::SelectOne(op.impl_->columns, op.impl_->select_all, op.impl_->conditions),
        ExtractFirstRow
    );
}

Rows DriverTableImpl::Execute(const operations::SelectMany& op) {
    auto& session = GetDriverSession(session_impl_);
    auto ctx = MakeTableRequestContext(session, "scylla_select_many", GetKeyspaceName(), GetTableName());
    const auto full_table = cql::MakeValidatedFullTableName(ctx.keyspace, ctx.table);

    auto
        stmt =
            cql::Prepare(
                ctx,
                cql::SelectMany(op.impl_->columns, op.impl_->select_all, op.impl_->conditions, op.impl_->limit, op.impl_->allow_filtering)(
                    full_table
                )
            );

    if (op.impl_->page_size > 0) {
        cass_statement_set_paging_size(stmt.get(), static_cast<int>(op.impl_->page_size));
    }

    auto result = ExecuteStatement(ctx, std::move(stmt), true, "SelectMany");
    return ExtractAllRows(result.get());
}

PagedRows DriverTableImpl::ExecutePaged(const operations::SelectMany& op, std::string paging_state) {
    auto& session = GetDriverSession(session_impl_);
    auto ctx = MakeTableRequestContext(session, "scylla_select_paged", GetKeyspaceName(), GetTableName());
    const auto full_table = cql::MakeValidatedFullTableName(ctx.keyspace, ctx.table);

    auto
        stmt =
            cql::Prepare(
                ctx,
                cql::SelectMany(op.impl_->columns, op.impl_->select_all, op.impl_->conditions, op.impl_->limit, op.impl_->allow_filtering)(
                    full_table
                )
            );

    const std::size_t page_size = op.impl_->page_size > 0 ? op.impl_->page_size : 1000;
    cass_statement_set_paging_size(stmt.get(), static_cast<int>(page_size));

    if (!paging_state.empty()) {
        if (cass_statement_set_paging_state_token(stmt.get(), paging_state.data(), paging_state.size()) != CASS_OK) {
            throw QueryException("ExecutePaged: invalid paging state token");
        }
    }

    auto result = ExecuteStatement(ctx, std::move(stmt), true, "SelectManyPaged");

    PagedRows out;
    out.rows = ExtractAllRows(result.get());
    out.has_more_pages = cass_result_has_more_pages(result.get()) == cass_true;
    if (out.has_more_pages) {
        const char* token = nullptr;
        std::size_t token_len = 0;
        cass_result_paging_state_token(result.get(), &token, &token_len);
        out.paging_state.assign(token, token_len);
    }
    return out;
}

void DriverTableImpl::Execute(const operations::DeleteOne& op) {
    RunTableRequest(
        GetDriverSession(session_impl_),
        "scylla_delete_one",
        GetKeyspaceName(),
        GetTableName(),
        "DeleteOne",
        true,
        cql::DeleteRows(op.impl_->conditions),
        kNoParse
    );
}

void DriverTableImpl::Execute(const operations::UpdateOne& op) {
    RunTableRequest(
        GetDriverSession(session_impl_),
        "scylla_update_one",
        GetKeyspaceName(),
        GetTableName(),
        "UpdateOne",
        true,
        cql::Update(op.impl_->assignments, op.impl_->conditions, op.impl_->using_clause),
        kNoParse
    );
}

std::int64_t DriverTableImpl::Execute(const operations::Count& op) {
    return RunTableRequest(
        GetDriverSession(session_impl_),
        "scylla_count",
        GetKeyspaceName(),
        GetTableName(),
        "Count",
        true,
        cql::Count(op.impl_->conditions),
        ExtractCount
    );
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
    auto ctx = MakeTableRequestContext(session, "scylla_insert_many", GetKeyspaceName(), GetTableName());
    const auto full_table = cql::MakeValidatedFullTableName(ctx.keyspace, ctx.table);

    CassBatchPtr batch{cass_batch_new(CASS_BATCH_TYPE_LOGGED)};
    for (const auto& row : rows) {
        auto stmt = cql::Prepare(ctx, cql::Insert(row, /*if_not_exists=*/false, op.impl_->using_clause)(full_table));
        if (cass_batch_add_statement(batch.get(), stmt.get()) != CASS_OK) {
            throw QueryException("InsertMany: failed to add statement to batch");
        }
    }

    ExecuteBatch(ctx, std::move(batch), true, "InsertMany");
}

operations::LwtResult DriverTableImpl::ExecuteLwt(const operations::InsertOne& op) {
    if (!op.impl_->if_not_exists) {
        throw QueryException("ExecuteLwt(InsertOne): IfNotExists() must be set");
    }
    return RunTableRequest(
        GetDriverSession(session_impl_),
        "scylla_insert_one_lwt",
        GetKeyspaceName(),
        GetTableName(),
        "InsertOneLwt",
        false,
        cql::Insert(op.impl_->bindings, /*if_not_exists=*/true, op.impl_->using_clause),
        ExtractLwtResult
    );
}

operations::LwtResult DriverTableImpl::ExecuteLwt(const operations::UpdateOne& op) {
    if (!op.impl_->if_exists && op.impl_->if_conditions.empty()) {
        throw QueryException("ExecuteLwt(UpdateOne): IfExists() or at least one If*() predicate is required");
    }
    return RunTableRequest(
        GetDriverSession(session_impl_),
        "scylla_update_one_lwt",
        GetKeyspaceName(),
        GetTableName(),
        "UpdateOneLwt",
        false,
        cql::UpdateLwt(
            op.impl_->assignments,
            op.impl_->conditions,
            op.impl_->if_conditions,
            op.impl_->if_exists,
            op.impl_->using_clause
        ),
        ExtractLwtResult
    );
}

operations::LwtResult DriverTableImpl::ExecuteLwt(const operations::DeleteOne& op) {
    if (!op.impl_->if_exists && op.impl_->if_conditions.empty()) {
        throw QueryException("ExecuteLwt(DeleteOne): IfExists() or at least one If*() predicate is required");
    }
    return RunTableRequest(
        GetDriverSession(session_impl_),
        "scylla_delete_one_lwt",
        GetKeyspaceName(),
        GetTableName(),
        "DeleteOneLwt",
        false,
        cql::DeleteRowsLwt(op.impl_->conditions, op.impl_->if_conditions, op.impl_->if_exists),
        ExtractLwtResult
    );
}

void DriverTableImpl::Execute(const operations::Truncate&) {
    auto& session = GetDriverSession(session_impl_);
    auto ctx = MakeTableRequestContext(session, "scylla_truncate", GetKeyspaceName(), GetTableName());

    ExecuteStatement(ctx, cql::BuildTruncateStatement(ctx), false, "Truncate");
}

}  // namespace storages::scylla::impl::driver

USERVER_NAMESPACE_END
