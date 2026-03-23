#include "table_impl.hpp"

#include <cassandra.h>

#include <string>
#include <variant>

#include <storages/scylla/driver/session_impl.hpp>
#include <storages/scylla/operations_impl.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::scylla::impl::driver {

DriverTableImpl::DriverTableImpl(SessionImplPtr session_impl, std::string keyspace_name, std::string table_name)
    : TableImpl(std::move(keyspace_name), std::move(table_name)), session_impl_(session_impl) {}

void DriverTableImpl::Execute(const operations::InsertOne& operation) {
    const auto& bindings = operation.impl_->bindings;

    if (bindings.empty()) {
        throw std::runtime_error("sorry no bindings");
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

    CassStatement* statement = cass_statement_new(query.c_str(), bindings.size());

    for (size_t i = 0; i < bindings.size(); ++i) {
        std::visit(
            [&](const auto& val) {
                using T = std::decay_t<decltype(val)>;

                if constexpr (std::is_same_v<T, std::string>) {
                    cass_statement_bind_string(statement, i, val.c_str());
                } else if constexpr (std::is_same_v<T, int32_t>) {
                    cass_statement_bind_int32(statement, i, val);
                } else if constexpr (std::is_same_v<T, int64_t>) {
                    cass_statement_bind_int64(statement, i, val);
                } else if constexpr (std::is_same_v<T, bool>) {
                    cass_statement_bind_bool(statement, i, val ? cass_true : cass_false);
                } else if constexpr (std::is_same_v<T, float>) {
                    cass_statement_bind_float(statement, i, val);
                } else if constexpr (std::is_same_v<T, double>) {
                    cass_statement_bind_double(statement, i, val);
                }
            },
            bindings[i].value
        );
    }

    auto* driver_session = dynamic_cast<DriverSessionImpl*>(session_impl_.get());

    CassFuture* future = cass_session_execute(driver_session->GetNativeSession(), statement);
    cass_future_wait(future);

    CassError rc = cass_future_error_code(future);
    if (rc != CASS_OK) {
        const char* message;
        size_t message_length;
        cass_future_error_message(future, &message, &message_length);
        std::string err(message, message_length);
        cass_future_free(future);
        cass_statement_free(statement);
        throw std::runtime_error("failed: " + err);
    }

    cass_future_free(future);
    cass_statement_free(statement);
}

}  // namespace storages::scylla::impl::driver

USERVER_NAMESPACE_END