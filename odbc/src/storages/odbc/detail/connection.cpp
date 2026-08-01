#include <storages/odbc/detail/connection.hpp>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <limits>
#include <string>
#include <vector>

#include <fmt/format.h>
#include <sql.h>
#include <sqlext.h>

#include <userver/tracing/span.hpp>
#include <userver/tracing/tags.hpp>

#include <storages/odbc/detail/broken_guard.hpp>
#include <storages/odbc/detail/deadline.hpp>
#include <storages/odbc/detail/diag_wrapper.hpp>
#include <storages/odbc/detail/result_wrapper.hpp>
#include <storages/odbc/detail/tracing.hpp>
#include <userver/storages/odbc/exception.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc {

namespace {

void DestroyEnvironmentHandle(SQLHENV handle) {
    if (handle != SQL_NULL_HENV) {
        SQLFreeHandle(SQL_HANDLE_ENV, handle);
    }
}

void DestroyDatabaseHandle(SQLHDBC handle) {
    if (handle != SQL_NULL_HDBC) {
        SQLDisconnect(handle);
        SQLFreeHandle(SQL_HANDLE_DBC, handle);
    }
}

Connection::EnvironmentHandle MakeEnvironmentHandle() {
    SQLHENV env = SQL_NULL_HENV;
    SQLRETURN ret = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &env);
    if (!SQL_SUCCEEDED(ret)) {
        throw ConnectionError(
            "Failed to allocate environment handle:" + detail::GetSQLDiagString(SQL_NULL_HANDLE, SQL_HANDLE_ENV)
        );
    }

    return Connection::EnvironmentHandle(env, &DestroyEnvironmentHandle);
}

Connection::DatabaseHandle MakeDatabaseHandle(SQLHENV env) {
    SQLHDBC dbc = SQL_NULL_HDBC;
    SQLRETURN ret = SQLAllocHandle(SQL_HANDLE_DBC, env, &dbc);
    if (!SQL_SUCCEEDED(ret)) {
        throw ConnectionError(
            "Failed to allocate connection handle:" + detail::GetSQLDiagString(SQL_NULL_HANDLE, SQL_HANDLE_DBC)
        );
    }

    return Connection::DatabaseHandle(dbc, &DestroyDatabaseHandle);
}

struct ParameterBinding final {
    SQLSMALLINT c_type;
    SQLSMALLINT sql_type;
    SQLULEN column_size;
    SQLPOINTER data;
    SQLLEN buffer_size;
};

struct BoundParameter final {
    using Value = std::variant<SQLCHAR, SQLBIGINT, SQLUBIGINT, SQLDOUBLE, std::string>;

    explicit BoundParameter(const impl::Parameter& parameter)
        : type{parameter.GetType()},
          is_null{parameter.IsNull()},
          value{MakeValue(parameter)}
    {}

    static Value MakeValue(const impl::Parameter& parameter) {
        using impl::ParameterType;
        switch (parameter.GetType()) {
            case ParameterType::kBoolean:
                return static_cast<SQLCHAR>(parameter.Get<bool>() ? 1 : 0);
            case ParameterType::kSignedInteger:
                return static_cast<SQLBIGINT>(parameter.Get<std::int64_t>());
            case ParameterType::kUnsignedInteger:
                return static_cast<SQLUBIGINT>(parameter.Get<std::uint64_t>());
            case ParameterType::kFloatingPoint:
                return static_cast<SQLDOUBLE>(parameter.Get<double>());
            case ParameterType::kString:
            case ParameterType::kUnknown:
                return parameter.Get<std::string>();
        }
        UINVARIANT(false, "Unknown ODBC parameter type");
    }

    impl::ParameterType type;
    bool is_null;
    Value value;
};

ParameterBinding GetParameterBinding(BoundParameter& parameter) {
    using impl::ParameterType;

    switch (parameter.type) {
        case ParameterType::kBoolean:
            return {
                SQL_C_BIT,
                SQL_BIT,
                1,
                &std::get<SQLCHAR>(parameter.value),
                static_cast<SQLLEN>(sizeof(SQLCHAR)),
            };
        case ParameterType::kSignedInteger:
            return {
                SQL_C_SBIGINT,
                SQL_BIGINT,
                19,
                &std::get<SQLBIGINT>(parameter.value),
                static_cast<SQLLEN>(sizeof(SQLBIGINT)),
            };
        case ParameterType::kUnsignedInteger:
            return {
                SQL_C_UBIGINT,
                SQL_BIGINT,
                20,
                &std::get<SQLUBIGINT>(parameter.value),
                static_cast<SQLLEN>(sizeof(SQLUBIGINT)),
            };
        case ParameterType::kFloatingPoint:
            return {
                SQL_C_DOUBLE,
                SQL_DOUBLE,
                15,
                &std::get<SQLDOUBLE>(parameter.value),
                static_cast<SQLLEN>(sizeof(SQLDOUBLE)),
            };
        case ParameterType::kString: {
            auto& string = std::get<std::string>(parameter.value);
            return {
                SQL_C_CHAR,
                SQL_VARCHAR,
                std::max<SQLULEN>(1, static_cast<SQLULEN>(string.size())),
                string.data(),
                static_cast<SQLLEN>(string.size()),
            };
        }
        case ParameterType::kUnknown:
            // SQLDescribeParam below replaces the SQL type. A dummy character
            // buffer keeps drivers that validate ValuePtr happy for NULL.
            return {
                SQL_C_CHAR,
                SQL_VARCHAR,
                1,
                std::get<std::string>(parameter.value).data(),
                0,
            };
    }
    UINVARIANT(false, "Unknown ODBC parameter type");
}

void BindParameters(SQLHSTMT statement, const impl::ParameterList& parameters, engine::Deadline deadline) {
    SQLSMALLINT expected_count = 0;
    const auto count_result = SQLNumParams(statement, &expected_count);
    if (!SQL_SUCCEEDED(count_result)) {
        throw StatementError(
            "Failed to determine ODBC parameter count:" + detail::GetSQLDiagString(statement, SQL_HANDLE_STMT)
        );
    }
    if (static_cast<std::size_t>(expected_count) != parameters.size()) {
        throw StatementError(
            fmt::format("ODBC parameter count mismatch: query expects {}, got {}", expected_count, parameters.size())
        );
    }

    std::vector<BoundParameter> bound_parameters;
    bound_parameters.reserve(parameters.size());
    for (const auto& parameter : parameters) {
        bound_parameters.emplace_back(parameter);
    }

    std::vector<SQLLEN> indicators(bound_parameters.size());
    for (std::size_t index = 0; index < bound_parameters.size(); ++index) {
        auto& parameter = bound_parameters[index];
        auto binding = GetParameterBinding(parameter);

        if (parameter.type == impl::ParameterType::kUnknown) {
            SQLSMALLINT decimal_digits = 0;
            SQLSMALLINT nullable = SQL_NULLABLE_UNKNOWN;
            const auto describe_result = SQLDescribeParam(
                statement,
                static_cast<SQLUSMALLINT>(index + 1),
                &binding.sql_type,
                &binding.column_size,
                &decimal_digits,
                &nullable
            );
            if (!SQL_SUCCEEDED(describe_result)) {
                throw StatementError(
                    "Cannot infer the type of a NULL ODBC parameter; add an explicit SQL cast or use a typed "
                    "std::optional where supported by the driver:" +
                    detail::GetSQLDiagString(statement, SQL_HANDLE_STMT)
                );
            }
        }

        indicators[index] = parameter.is_null ? SQL_NULL_DATA : binding.buffer_size;
        const auto bind_result = SQLBindParameter(
            statement,
            static_cast<SQLUSMALLINT>(index + 1),
            SQL_PARAM_INPUT,
            binding.c_type,
            binding.sql_type,
            binding.column_size,
            0,
            binding.data,
            binding.buffer_size,
            &indicators[index]
        );
        if (!SQL_SUCCEEDED(bind_result)) {
            throw StatementError(
                fmt::format("Failed to bind ODBC parameter {}:", index + 1) +
                detail::GetSQLDiagString(statement, SQL_HANDLE_STMT)
            );
        }
    }

    detail::CheckDeadlineNotExpired(deadline);
    const auto execute_result = SQLExecute(statement);
    detail::CheckDeadlineNotExpired(deadline);
    if (!SQL_SUCCEEDED(execute_result) && execute_result != SQL_NO_DATA) {
        throw StatementError(
            "Failed to execute prepared ODBC query:" + detail::GetSQLDiagString(statement, SQL_HANDLE_STMT)
        );
    }
}

SQLRETURN ExecuteStatement(
    SQLHSTMT statement,
    std::string_view query,
    const impl::ParameterList& parameters,
    engine::Deadline deadline
) {
    std::vector<SQLCHAR> query_buffer(query.begin(), query.end());
    query_buffer.push_back('\0');

    if (parameters.empty()) {
        detail::CheckDeadlineNotExpired(deadline);
        const auto execute_result = SQLExecDirect(statement, query_buffer.data(), SQL_NTS);
        detail::CheckDeadlineNotExpired(deadline);
        return execute_result;
    }

    detail::CheckDeadlineNotExpired(deadline);
    const auto prepare_result = SQLPrepare(statement, query_buffer.data(), SQL_NTS);
    detail::CheckDeadlineNotExpired(deadline);
    if (!SQL_SUCCEEDED(prepare_result)) {
        throw StatementError("Failed to prepare ODBC query:" + detail::GetSQLDiagString(statement, SQL_HANDLE_STMT));
    }
    BindParameters(statement, parameters, deadline);
    return SQL_SUCCESS;
}

}  // namespace

Connection::Connection(const std::string& dsn)
    : Connection{dsn, detail::GetExecuteDeadline(detail::kDefaultStatementTimeout)}
{}

Connection::Connection(const std::string& dsn, engine::Deadline deadline)
    : env_(MakeEnvironmentHandle()),
      handle_(Connection::DatabaseHandle(SQL_NULL_HDBC, &DestroyDatabaseHandle))
{
    detail::CheckDeadlineNotExpired(deadline);
    SQLRETURN ret =
        SQLSetEnvAttr(env_.get(), SQL_ATTR_CONNECTION_POOLING, reinterpret_cast<SQLPOINTER>(SQL_CP_ONE_PER_DRIVER), 0);
    if (!SQL_SUCCEEDED(ret)) {
        throw ConnectionError(
            "Failed to set connection pooling attribute:" + detail::GetSQLDiagString(env_.get(), SQL_HANDLE_ENV)
        );
    }

    ret = SQLSetEnvAttr(env_.get(), SQL_ATTR_ODBC_VERSION, reinterpret_cast<SQLPOINTER>(SQL_OV_ODBC3), 0);
    if (!SQL_SUCCEEDED(ret)) {
        throw ConnectionError("Failed to set ODBC version:" + detail::GetSQLDiagString(env_.get(), SQL_HANDLE_ENV));
    }

    handle_ = MakeDatabaseHandle(env_.get());

    if (deadline.IsReachable()) {
        const auto time_left = deadline.TimeLeft();
        if (time_left <= engine::Deadline::Duration::zero()) {
            detail::CheckDeadlineNotExpired(deadline);
        }
        const auto timeout = std::chrono::ceil<std::chrono::seconds>(time_left);
        const auto timeout_seconds = static_cast<SQLULEN>(timeout.count());
        ret = SQLSetConnectAttr(
            handle_.get(),
            SQL_ATTR_LOGIN_TIMEOUT,
            reinterpret_cast<SQLPOINTER>(static_cast<std::uintptr_t>(timeout_seconds)),
            SQL_IS_UINTEGER
        );
        if (!SQL_SUCCEEDED(ret)) {
            throw ConnectionError(
                "Failed to set ODBC login timeout:" + detail::GetSQLDiagString(handle_.get(), SQL_HANDLE_DBC)
            );
        }
    }

    std::vector<SQLCHAR> dsn_buffer(dsn.begin(), dsn.end());
    dsn_buffer.push_back('\0');
    detail::CheckDeadlineNotExpired(deadline);
    ret =
        SQLDriverConnect(handle_.get(), nullptr, dsn_buffer.data(), SQL_NTS, nullptr, 0, nullptr, SQL_DRIVER_COMPLETE);
    detail::CheckDeadlineNotExpired(deadline);
    if (!SQL_SUCCEEDED(ret)) {
        throw ConnectionError(
            "Failed to connect to database: " + detail::GetSQLDiagString(handle_.get(), SQL_HANDLE_DBC)
        );
    }
    SQLUINTEGER scroll_option = 0;
    ret = SQLGetInfo(handle_.get(), SQL_SCROLL_OPTIONS, &scroll_option, sizeof(scroll_option), nullptr);
    if (!SQL_SUCCEEDED(ret)) {
        throw ConnectionError(
            "Failed to get scroll options:" + detail::GetSQLDiagString(handle_.get(), SQL_HANDLE_DBC)
        );
    }

    // TODO: add support for other scroll options
    if (!(scroll_option & SQL_FD_FETCH_ABSOLUTE)) {
        throw ConnectionError("SQL_FD_FETCH_ABSOLUTE is not supported");
    }
}

ResultSet Connection::Query(std::string_view query) {
    return Query(query, impl::ParameterList{}, detail::GetExecuteDeadline(detail::kDefaultStatementTimeout));
}

ResultSet Connection::Query(std::string_view query, engine::Deadline deadline) {
    return Query(query, impl::ParameterList{}, deadline);
}

ResultSet Connection::Query(std::string_view query, const impl::ParameterList& parameters) {
    return Query(query, parameters, detail::GetExecuteDeadline(detail::kDefaultStatementTimeout));
}

ResultSet Connection::Query(std::string_view query, const impl::ParameterList& parameters, engine::Deadline deadline) {
    detail::CheckDeadlineNotExpired(deadline);

    auto guard = GetBrokenGuard();
    return guard.Execute([&] {
        tracing::Span span{detail::tracing::MakeQuerySpanName(query)};
        span.AddTag(tracing::kDatabaseType, "odbc");
        span.AddTag(tracing::kDatabaseStatement, std::string{query});

        auto stmt = detail::MakeResultHandle(handle_.get());

        if (deadline.IsReachable()) {
            const auto left = deadline.TimeLeft();
            if (left <= engine::Deadline::Duration::zero()) {
                detail::CheckDeadlineNotExpired(deadline);
            }
            const auto seconds = std::chrono::ceil<std::chrono::seconds>(left);
            const auto timeout_sec = static_cast<SQLULEN>(seconds.count());
            /* ODBC SQL_ATTR_QUERY_TIMEOUT is in whole seconds; deadline checks still use full TimeLeft()
             * resolution. */
            const auto timeout_result = SQLSetStmtAttr(
                stmt.get(),
                SQL_ATTR_QUERY_TIMEOUT,
                reinterpret_cast<SQLPOINTER>(static_cast<std::uintptr_t>(timeout_sec)),
                0
            );
            if (!SQL_SUCCEEDED(timeout_result)) {
                throw StatementError(
                    "Failed to set ODBC query timeout:" + detail::GetSQLDiagString(stmt.get(), SQL_HANDLE_STMT)
                );
            }
        }

        SQLRETURN ret = ExecuteStatement(stmt.get(), query, parameters, deadline);
        if (!SQL_SUCCEEDED(ret) && ret != SQL_NO_DATA) {
            const auto diag = detail::GetSQLDiagString(stmt.get(), SQL_HANDLE_STMT);
            span.AddTag(tracing::kErrorFlag, true);
            span.AddTag(tracing::kErrorMessage, diag);
            throw StatementError("Failed to execute query:" + diag);
        }

        // Only call Fetch for SELECT-like statements that produce a result set.
        // DML statements (INSERT/UPDATE/DELETE) have 0 result columns; calling
        // SQLFetch on them returns SQL_NO_DATA or an error depending on the driver.
        if (ret != SQL_NO_DATA) {
            SQLSMALLINT col_count = 0;
            SQLNumResultCols(stmt.get(), &col_count);
            if (col_count > 0) {
                auto wrapper = std::make_shared<detail::ResultWrapper>(std::move(stmt));
                wrapper->Fetch();
                return ResultSet(std::move(wrapper));
            }
        }

        return ResultSet(std::make_shared<detail::ResultWrapper>(std::move(stmt)));
    });
}

bool Connection::DriverReportsDead() const {
    SQLUINTEGER state = 0;
    SQLRETURN ret = SQLGetConnectAttr(handle_.get(), SQL_ATTR_CONNECTION_DEAD, &state, sizeof(state), nullptr);
    if (!SQL_SUCCEEDED(ret) || state == SQL_CD_TRUE) {
        return true;
    }

    return false;
}

bool Connection::IsBroken() const { return broken_.load() || DriverReportsDead(); }

void Connection::NotifyBroken() { broken_.store(true); }

detail::BrokenGuard Connection::GetBrokenGuard() { return detail::BrokenGuard{*this}; }

bool Connection::IsInsideTransaction() const {
    SQLUINTEGER state = 0;
    SQLRETURN ret = SQLGetConnectAttr(handle_.get(), SQL_ATTR_AUTOCOMMIT, &state, sizeof(state), nullptr);
    if (!SQL_SUCCEEDED(ret) || state == SQL_AUTOCOMMIT_OFF) {
        return true;
    }
    return false;
}

void Connection::Begin(engine::Deadline deadline) {
    auto guard = GetBrokenGuard();
    guard.Execute([this, deadline] {
        detail::CheckDeadlineNotExpired(deadline);
        SQLRETURN ret = SQLSetConnectAttr(
            handle_.get(),
            SQL_ATTR_AUTOCOMMIT,
            reinterpret_cast<SQLPOINTER>(SQL_AUTOCOMMIT_OFF),
            SQL_IS_UINTEGER
        );

        if (!SQL_SUCCEEDED(ret)) {
            throw ConnectionError(
                "Failed to set connection autocommit attribute:" +
                detail::GetSQLDiagString(handle_.get(), SQL_HANDLE_DBC)
            );
        }
    });
}

void Connection::Commit(engine::Deadline deadline) {
    auto guard = GetBrokenGuard();
    guard.Execute([this, deadline] {
        detail::CheckDeadlineNotExpired(deadline);
        if (!IsInsideTransaction()) {
            throw ConnectionError(
                "User try to commit autocommit connection:" + detail::GetSQLDiagString(handle_.get(), SQL_HANDLE_DBC)
            );
        }
        SQLRETURN ret = SQLEndTran(SQL_HANDLE_DBC, handle_.get(), SQL_COMMIT);
        if (!SQL_SUCCEEDED(ret)) {
            throw ConnectionError(
                "Failed to commit transaction inside connection:" +
                detail::GetSQLDiagString(handle_.get(), SQL_HANDLE_DBC)
            );
        }
        RestoreAutocommit();
    });
}

void Connection::Rollback(engine::Deadline deadline) {
    auto guard = GetBrokenGuard();
    guard.Execute([this, deadline] {
        detail::CheckDeadlineNotExpired(deadline);
        if (!IsInsideTransaction()) {
            throw ConnectionError(
                "User try to rollback autocommit connection:" + detail::GetSQLDiagString(handle_.get(), SQL_HANDLE_DBC)
            );
        }
        SQLRETURN ret = SQLEndTran(SQL_HANDLE_DBC, handle_.get(), SQL_ROLLBACK);
        if (!SQL_SUCCEEDED(ret)) {
            throw ConnectionError(
                "Failed to rollback transaction inside connection:" +
                detail::GetSQLDiagString(handle_.get(), SQL_HANDLE_DBC)
            );
        }
        RestoreAutocommit();
    });
}

void Connection::RestoreAutocommit() {
    SQLRETURN ret = SQLSetConnectAttr(
        handle_.get(),
        SQL_ATTR_AUTOCOMMIT,
        reinterpret_cast<SQLPOINTER>(SQL_AUTOCOMMIT_ON),
        SQL_IS_UINTEGER
    );
    if (!SQL_SUCCEEDED(ret)) {
        throw ConnectionError(
            "Failed to restore autocommit after transaction:" + detail::GetSQLDiagString(handle_.get(), SQL_HANDLE_DBC)
        );
    }
}

}  // namespace storages::odbc

USERVER_NAMESPACE_END
