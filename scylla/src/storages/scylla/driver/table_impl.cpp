#include "table_impl.hpp"

#include <cassandra.h>

#include <optional>
#include <string>
#include <variant>

#include <storages/scylla/driver/cass_wrappers.hpp>
#include <storages/scylla/driver/scylla_error.hpp>
#include <storages/scylla/driver/session_impl.hpp>
#include <storages/scylla/operations_impl.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::scylla::impl::driver {

DriverTableImpl::DriverTableImpl(SessionImplPtr session_impl, std::string keyspace_name, std::string table_name)
    : TableImpl(std::move(keyspace_name), std::move(table_name)), session_impl_(session_impl) {}

void DriverTableImpl::Execute(const operations::InsertOne& operation) {
    const auto& bindings = operation.impl_->bindings;

    if (bindings.empty()) {
        throw QueryException("InsertOne: no bindings provided");
    }

    std::string columns, placeholders;
    for (size_t i = 0; i < bindings.size(); ++i) {
        if (i > 0) {
            columns += ", ";
            placeholders += ", ";
        }
        columns += bindings[i].column_name;
        placeholders += "?";
    }

    std::string full_table;

    if (!GetKeyspaceName().empty()) {
        full_table = GetKeyspaceName() + "." + GetTableName();
    } else {
        full_table = GetTableName();
    }

    std::string query = "INSERT INTO " + full_table + " (" + columns + ") VALUES (" + placeholders + ")";

    CassStatementPtr statement(cass_statement_new(query.c_str(), bindings.size()));

    for (size_t i = 0; i < bindings.size(); ++i) {
        std::visit(
            [&](const auto& val) {
                using T = std::decay_t<decltype(val)>;

                if constexpr (std::is_same_v<T, std::string>) {
                    cass_statement_bind_string(statement.get(), i, val.c_str());
                } else if constexpr (std::is_same_v<T, int32_t>) {
                    cass_statement_bind_int32(statement.get(), i, val);
                } else if constexpr (std::is_same_v<T, int64_t>) {
                    cass_statement_bind_int64(statement.get(), i, val);
                } else if constexpr (std::is_same_v<T, bool>) {
                    cass_statement_bind_bool(statement.get(), i, val ? cass_true : cass_false);
                } else if constexpr (std::is_same_v<T, float>) {
                    cass_statement_bind_float(statement.get(), i, val);
                } else if constexpr (std::is_same_v<T, double>) {
                    cass_statement_bind_double(statement.get(), i, val);
                }
            },
            bindings[i].value
        );
    }

    auto* driver_session = dynamic_cast<DriverSessionImpl*>(session_impl_.get());

    CassFuturePtr future(cass_session_execute(driver_session->GetNativeSession(), statement.get()));
    cass_future_wait(future.get());
    CheckFuture(future.get(), "InsertOne");
}

operations::SelectOne::Row DriverTableImpl::Execute(const operations::SelectOne& operation) {
    const auto& conditions = operation.impl_->conditions;
    const auto& columns = operation.impl_->columns;
    const bool select_all = operation.impl_->select_all;

    std::string full_table;
    if (!GetKeyspaceName().empty()) {
        full_table = GetKeyspaceName() + "." + GetTableName();
    } else {
        full_table = GetTableName();
    }

    std::string cols_str;
    if (select_all) {
        cols_str = "*";
    } else {
        for (size_t i = 0; i < columns.size(); ++i) {
            if (i > 0) cols_str += ", ";
            cols_str += columns[i];
        }
    }

    std::string query = "SELECT " + cols_str + " FROM " + full_table;

    if (!conditions.empty()) {
        query += " WHERE ";
        for (size_t i = 0; i < conditions.size(); ++i) {
            if (i > 0) query += " AND ";
            query += conditions[i].column_name + " = ?";
        }
    }

    query += " LIMIT 1";

    CassStatementPtr statement(cass_statement_new(query.c_str(), conditions.size()));

    for (size_t i = 0; i < conditions.size(); ++i) {
        std::visit(
            [&](const auto& val) {
                using T = std::decay_t<decltype(val)>;

                if constexpr (std::is_same_v<T, std::string>) {
                    cass_statement_bind_string(statement.get(), i, val.c_str());
                } else if constexpr (std::is_same_v<T, int32_t>) {
                    cass_statement_bind_int32(statement.get(), i, val);
                } else if constexpr (std::is_same_v<T, int64_t>) {
                    cass_statement_bind_int64(statement.get(), i, val);
                } else if constexpr (std::is_same_v<T, bool>) {
                    cass_statement_bind_bool(statement.get(), i, val ? cass_true : cass_false);
                } else if constexpr (std::is_same_v<T, float>) {
                    cass_statement_bind_float(statement.get(), i, val);
                } else if constexpr (std::is_same_v<T, double>) {
                    cass_statement_bind_double(statement.get(), i, val);
                }
            },
            conditions[i].value
        );
    }

    auto* driver_session = dynamic_cast<DriverSessionImpl*>(session_impl_.get());

    CassFuturePtr future(cass_session_execute(driver_session->GetNativeSession(), statement.get()));
    cass_future_wait(future.get());
    CheckFuture(future.get(), "SelectOne");

    const CassResult* result = cass_future_get_result(future.get());
    operations::SelectOne::Row row;

    const CassRow* cass_row = cass_result_first_row(result);
    if (cass_row) {
        size_t col_count = cass_result_column_count(result);
        for (size_t i = 0; i < col_count; ++i) {
            const char* col_name = nullptr;
            size_t col_name_length = 0;
            cass_result_column_name(result, i, &col_name, &col_name_length);
            std::string name(col_name, col_name_length);

            const CassValue* cass_val = cass_row_get_column(cass_row, i);
            CassValueType val_type = cass_value_type(cass_val);

            std::optional<operations::SelectOne::Value> value;
            switch (val_type) {
                case CASS_VALUE_TYPE_VARCHAR:
                case CASS_VALUE_TYPE_TEXT: {
                    const char* str = nullptr;
                    size_t str_length = 0;
                    cass_value_get_string(cass_val, &str, &str_length);
                    value = std::string(str, str_length);
                    break;
                }
                case CASS_VALUE_TYPE_INT: {
                    int32_t v = 0;
                    cass_value_get_int32(cass_val, &v);
                    value = v;
                    break;
                }
                case CASS_VALUE_TYPE_BIGINT: {
                    int64_t v = 0;
                    cass_value_get_int64(cass_val, &v);
                    value = v;
                    break;
                }
                case CASS_VALUE_TYPE_BOOLEAN: {
                    cass_bool_t v = cass_false;
                    cass_value_get_bool(cass_val, &v);
                    value = static_cast<bool>(v);
                    break;
                }
                case CASS_VALUE_TYPE_FLOAT: {
                    float v = 0.0f;
                    cass_value_get_float(cass_val, &v);
                    value = v;
                    break;
                }
                case CASS_VALUE_TYPE_DOUBLE: {
                    double v = 0.0;
                    cass_value_get_double(cass_val, &v);
                    value = v;
                    break;
                }
                default:
                    continue;
            }

            row.emplace_back(std::move(name), std::move(*value));
        }
    }

    cass_result_free(result);

    return row;
}

}  // namespace storages::scylla::impl::driver

USERVER_NAMESPACE_END