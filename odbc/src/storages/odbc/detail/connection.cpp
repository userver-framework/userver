#include <storages/odbc/detail/connection.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <exception>
#include <functional>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <fmt/format.h>
#include <sql.h>
#include <sqlext.h>

#include <userver/tracing/span.hpp>
#include <userver/tracing/tags.hpp>

#include <userver/engine/async.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/engine/task/current_task.hpp>
#include <userver/logging/log.hpp>

#include <storages/odbc/detail/broken_guard.hpp>
#include <storages/odbc/detail/deadline.hpp>
#include <storages/odbc/detail/diag_wrapper.hpp>
#include <storages/odbc/detail/result_wrapper.hpp>
#include <storages/odbc/detail/tracing.hpp>
#include <storages/odbc/detail/transaction_options.hpp>
#include <userver/storages/odbc/exception.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc {

namespace {

using StatementHandle = std::unique_ptr<std::remove_pointer_t<SQLHSTMT>, void (*)(SQLHSTMT)>;

struct ConnectedHandles final {
    Connection::EnvironmentHandle environment;
    Connection::DatabaseHandle database;
    detail::DriverCapabilities driver_capabilities;
};

template <typename Func>
auto RunBlocking(engine::TaskProcessor& task_processor, Func&& func) -> std::invoke_result_t<Func> {
    const engine::TaskCancellationBlocker cancellation_blocker;
    auto task = engine::CriticalAsyncNoTracing(task_processor, std::forward<Func>(func));
    return task.Get();
}

void CheckOperationInterrupted(engine::Deadline deadline) {
    detail::CheckDeadlineNotExpired(deadline);
    if (engine::current_task::ShouldCancel()) {
        throw OperationInterrupted("Cancelled by task cancellation");
    }
}

template <typename Func>
auto RunBlockingChecked(engine::TaskProcessor& task_processor, engine::Deadline deadline, Func&& func)
    -> std::invoke_result_t<Func> {
    CheckOperationInterrupted(deadline);
    try {
        if constexpr (std::is_void_v<std::invoke_result_t<Func>>) {
            RunBlocking(task_processor, std::forward<Func>(func));
            CheckOperationInterrupted(deadline);
        } else {
            auto result = RunBlocking(task_processor, std::forward<Func>(func));
            CheckOperationInterrupted(deadline);
            return result;
        }
    } catch (...) {
        // Run the deadline check in the caller task so inherited-deadline
        // accounting is applied to the request, not to the blocking worker.
        CheckOperationInterrupted(deadline);
        throw;
    }
}

void ConfigureDriverManager() {
    static std::once_flag flag;
    std::call_once(flag, [] {
        const auto result =
            SQLSetEnvAttr(SQL_NULL_HANDLE, SQL_ATTR_CONNECTION_POOLING, reinterpret_cast<SQLPOINTER>(SQL_CP_OFF), 0);
        if (!SQL_SUCCEEDED(result)) {
            throw ConnectionError("Failed to disable ODBC driver-manager connection pooling");
        }
    });
}

template <typename Exception>
Exception MakeDriverError(std::string message, SQLRETURN result, SQLHANDLE handle, SQLSMALLINT handle_type) {
    auto diagnostics = detail::GetSQLDiagnostics(handle, handle_type);
    const auto formatted = detail::FormatSQLDiagnostics(diagnostics);
    if (!formatted.empty()) {
        message += ": ";
        message += formatted;
    }
    return Exception{std::move(message), std::move(diagnostics), result == SQL_INVALID_HANDLE};
}

ConnectionError MakeConnectionError(std::string message, SQLRETURN result, std::vector<DiagnosticRecord> diagnostics) {
    const auto formatted = detail::FormatSQLDiagnostics(diagnostics);
    if (!formatted.empty()) {
        message += ": ";
        message += formatted;
    }
    return ConnectionError{std::move(message), std::move(diagnostics), result == SQL_INVALID_HANDLE};
}

void LogConnectionAttributeWarnings(std::string_view operation, const std::vector<DiagnosticRecord>& diagnostics) {
    constexpr std::size_t kMaxWarningLength = 1024;
    auto formatted = detail::FormatSQLDiagnostics(diagnostics);
    if (formatted.size() > kMaxWarningLength) {
        formatted.resize(kMaxWarningLength);
        formatted += "...";
    }
    LOG_WARNING()
        << operation << " completed with warning: " << (formatted.empty() ? "no diagnostic records" : formatted);
}

SQLUINTEGER ReadConnectionAttribute(
    SQLHDBC connection,
    SQLINTEGER attribute,
    std::string_view attribute_name,
    engine::Deadline deadline
) {
    detail::CheckDeadlineNotExpired(deadline);
    SQLUINTEGER value = 0;
    SQLINTEGER value_size = 0;
    const auto
        result = SQLGetConnectAttr(connection, attribute, &value, static_cast<SQLINTEGER>(sizeof(value)), &value_size);
    std::exception_ptr deadline_error;
    try {
        detail::CheckDeadlineNotExpired(deadline);
    } catch (...) {
        deadline_error = std::current_exception();
    }

    auto diagnostics =
        result == SQL_SUCCESS ? std::vector<DiagnosticRecord>{} : detail::GetSQLDiagnostics(connection, SQL_HANDLE_DBC);
    if (!SQL_SUCCEEDED(result)) {
        throw MakeConnectionError(
            fmt::format("Failed to read ODBC connection attribute {}", attribute_name),
            result,
            std::move(diagnostics)
        );
    }
    if (std::any_of(diagnostics.begin(), diagnostics.end(), [](const DiagnosticRecord& diagnostic) {
            return diagnostic.sql_state == "01004";
        }))
    {
        throw MakeConnectionError(
            fmt::format("Failed to read exact ODBC connection attribute {}", attribute_name),
            result,
            std::move(diagnostics)
        );
    }
    if (deadline_error) {
        std::rethrow_exception(deadline_error);
    }
    if (result == SQL_SUCCESS_WITH_INFO) {
        LogConnectionAttributeWarnings(
            fmt::format("Reading ODBC connection attribute {}", attribute_name),
            diagnostics
        );
    }
    return value;
}

void SetConnectionAttributeVerified(
    SQLHDBC connection,
    SQLINTEGER attribute,
    SQLUINTEGER requested_value,
    std::string_view attribute_name,
    engine::Deadline deadline
) {
    detail::CheckDeadlineNotExpired(deadline);
    const auto result = SQLSetConnectAttr(
        connection,
        attribute,
        reinterpret_cast<SQLPOINTER>(static_cast<std::uintptr_t>(requested_value)),
        SQL_IS_UINTEGER
    );
    std::exception_ptr deadline_error;
    try {
        detail::CheckDeadlineNotExpired(deadline);
    } catch (...) {
        deadline_error = std::current_exception();
    }

    auto diagnostics =
        result == SQL_SUCCESS ? std::vector<DiagnosticRecord>{} : detail::GetSQLDiagnostics(connection, SQL_HANDLE_DBC);
    if (!SQL_SUCCEEDED(result)) {
        throw MakeConnectionError(
            fmt::format("Failed to set requested ODBC connection attribute {}", attribute_name),
            result,
            std::move(diagnostics)
        );
    }
    if (deadline_error) {
        std::rethrow_exception(deadline_error);
    }

    const auto actual_value = ReadConnectionAttribute(connection, attribute, attribute_name, deadline);
    if (!detail::IsExactConnectionAttributeValue(requested_value, actual_value)) {
        throw MakeConnectionError(
            fmt::format(
                "ODBC driver substituted connection attribute {} value {} with {}",
                attribute_name,
                requested_value,
                actual_value
            ),
            result,
            std::move(diagnostics)
        );
    }
    if (result == SQL_SUCCESS_WITH_INFO) {
        LogConnectionAttributeWarnings(
            fmt::format("Setting ODBC connection attribute {}", attribute_name),
            diagnostics
        );
    }
}

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

void DestroyStatementHandle(SQLHSTMT handle) {
    if (handle != SQL_NULL_HSTMT) {
        SQLFreeHandle(SQL_HANDLE_STMT, handle);
    }
}

void DestroyConnectionHandlesOnBlockingTaskProcessor(
    engine::TaskProcessor& task_processor,
    Connection::EnvironmentHandle& environment,
    Connection::DatabaseHandle& database
) noexcept {
    // Release before scheduling so a task-scheduling failure cannot invoke
    // synchronous ODBC cleanup on the caller coroutine task processor.
    const auto database_handle = database.release();
    const auto environment_handle = environment.release();
    try {
        RunBlocking(task_processor, [database_handle, environment_handle] {
            DestroyDatabaseHandle(database_handle);
            DestroyEnvironmentHandle(environment_handle);
        });
    } catch (const std::exception& ex) {
        // At this point blocking in the caller is worse than leaking two
        // handles while task processors are already unable to accept cleanup.
        LOG_ERROR() << "Failed to schedule ODBC connection cleanup on blocking task processor: " << ex;
    } catch (...) {
        LOG_ERROR() << "Failed to schedule ODBC connection cleanup on blocking task processor";
    }
}

Connection::EnvironmentHandle MakeEnvironmentHandle() {
    SQLHENV env = SQL_NULL_HENV;
    SQLRETURN ret = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &env);
    if (!SQL_SUCCEEDED(ret)) {
        throw MakeDriverError<
            ConnectionError>("Failed to allocate environment handle", ret, SQL_NULL_HANDLE, SQL_HANDLE_ENV);
    }

    return Connection::EnvironmentHandle(env, &DestroyEnvironmentHandle);
}

Connection::DatabaseHandle MakeDatabaseHandle(SQLHENV env) {
    SQLHDBC dbc = SQL_NULL_HDBC;
    SQLRETURN ret = SQLAllocHandle(SQL_HANDLE_DBC, env, &dbc);
    if (!SQL_SUCCEEDED(ret)) {
        throw MakeDriverError<ConnectionError>("Failed to allocate connection handle", ret, env, SQL_HANDLE_ENV);
    }

    return Connection::DatabaseHandle(dbc, &DestroyDatabaseHandle);
}

StatementHandle MakeStatementHandle(SQLHDBC connection) {
    SQLHSTMT statement = SQL_NULL_HSTMT;
    const auto result = SQLAllocHandle(SQL_HANDLE_STMT, connection, &statement);
    if (!SQL_SUCCEEDED(result)) {
        throw MakeDriverError<
            StatementError>("Failed to allocate ODBC statement handle", result, connection, SQL_HANDLE_DBC);
    }
    return StatementHandle{statement, &DestroyStatementHandle};
}

void CheckStatementResult(SQLRETURN result, SQLHSTMT statement, std::string_view operation) {
    if (!SQL_SUCCEEDED(result)) {
        throw MakeDriverError<
            StatementError>(fmt::format("Failed to {} ODBC statement", operation), result, statement, SQL_HANDLE_STMT);
    }
}

detail::ResultWrapper::Column DescribeColumn(SQLHSTMT statement, SQLUSMALLINT column) {
    std::array<SQLCHAR, 256> buffer{};
    SQLSMALLINT name_length = 0;
    SQLSMALLINT type = SQL_UNKNOWN_TYPE;
    auto result = SQLDescribeCol(
        statement,
        column,
        buffer.data(),
        static_cast<SQLSMALLINT>(buffer.size()),
        &name_length,
        &type,
        nullptr,
        nullptr,
        nullptr
    );
    CheckStatementResult(result, statement, "describe result column");

    if (name_length >= static_cast<SQLSMALLINT>(buffer.size())) {
        std::vector<SQLCHAR> long_buffer(static_cast<std::size_t>(name_length) + 1);
        result = SQLDescribeCol(
            statement,
            column,
            long_buffer.data(),
            static_cast<SQLSMALLINT>(long_buffer.size()),
            &name_length,
            &type,
            nullptr,
            nullptr,
            nullptr
        );
        CheckStatementResult(result, statement, "describe result column");
        return {
            std::string{reinterpret_cast<const char*>(long_buffer.data()), static_cast<std::size_t>(name_length)},
            type,
        };
    }

    return {
        std::string{reinterpret_cast<const char*>(buffer.data()), static_cast<std::size_t>(name_length)},
        type,
    };
}

detail::ResultWrapper::Cell ReadCell(SQLHSTMT statement, SQLUSMALLINT column, engine::Deadline deadline) {
    constexpr std::size_t kChunkSize = 4096;
    std::array<SQLCHAR, kChunkSize> buffer{};
    std::string value;

    while (true) {
        SQLLEN indicator = 0;
        detail::CheckDeadlineNotExpired(deadline);
        const auto result =
            SQLGetData(statement, column, SQL_C_CHAR, buffer.data(), static_cast<SQLLEN>(buffer.size()), &indicator);
        detail::CheckDeadlineNotExpired(deadline);

        if (result == SQL_NO_DATA) {
            break;
        }
        CheckStatementResult(result, statement, "read result column");
        if (indicator == SQL_NULL_DATA) {
            return {std::nullopt};
        }

        const auto terminator = std::find(buffer.begin(), buffer.end(), static_cast<SQLCHAR>('\0'));
        const auto chunk_size = static_cast<std::size_t>(terminator - buffer.begin());
        value.append(reinterpret_cast<const char*>(buffer.data()), chunk_size);

        if (result == SQL_SUCCESS) {
            break;
        }
        if (chunk_size == 0) {
            throw ResultSetError("ODBC driver returned SQL_SUCCESS_WITH_INFO without result data progress");
        }
    }

    return {std::move(value)};
}

std::shared_ptr<detail::ResultWrapper> MaterializeResult(SQLHSTMT statement, engine::Deadline deadline) {
    SQLSMALLINT column_count = 0;
    CheckStatementResult(SQLNumResultCols(statement, &column_count), statement, "get result column count for");

    std::size_t rows_affected = 0;
    if (column_count == 0) {
        SQLLEN affected = 0;
        const auto row_count_result = SQLRowCount(statement, &affected);
        rows_affected = SQL_SUCCEEDED(row_count_result) && affected > 0 ? static_cast<std::size_t>(affected) : 0;
    }

    std::vector<detail::ResultWrapper::Column> columns;
    columns.reserve(static_cast<std::size_t>(column_count));
    for (SQLSMALLINT index = 0; index < column_count; ++index) {
        columns.push_back(DescribeColumn(statement, static_cast<SQLUSMALLINT>(index + 1)));
    }

    std::vector<detail::ResultWrapper::Row> rows;
    if (column_count > 0) {
        while (true) {
            detail::CheckDeadlineNotExpired(deadline);
            const auto fetch_result = SQLFetch(statement);
            detail::CheckDeadlineNotExpired(deadline);
            if (fetch_result == SQL_NO_DATA) {
                break;
            }
            CheckStatementResult(fetch_result, statement, "fetch row from");

            detail::ResultWrapper::Row row;
            row.reserve(static_cast<std::size_t>(column_count));
            for (SQLSMALLINT index = 0; index < column_count; ++index) {
                row.push_back(ReadCell(statement, static_cast<SQLUSMALLINT>(index + 1), deadline));
            }
            rows.push_back(std::move(row));
        }
    }

    return std::make_shared<detail::ResultWrapper>(std::move(columns), std::move(rows), rows_affected);
}

struct ParameterBinding final {
    SQLSMALLINT c_type;
    SQLSMALLINT sql_type;
    SQLULEN column_size;
    SQLPOINTER data;
    SQLLEN buffer_size;
};

struct BoundParameter final {
    using Value = std::variant<SQLCHAR, SQLBIGINT, SQLDOUBLE, std::string>;

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
            case ParameterType::kUnsignedInteger: {
                const auto value = parameter.Get<std::uint64_t>();
                if (value > static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max())) {
                    throw StatementError("ODBC unsigned integer parameter is outside the portable SQL BIGINT range");
                }
                return static_cast<SQLBIGINT>(value);
            }
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
                SQL_C_SBIGINT,
                SQL_BIGINT,
                19,
                &std::get<SQLBIGINT>(parameter.value),
                static_cast<SQLLEN>(sizeof(SQLBIGINT)),
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
        throw MakeDriverError<
            StatementError>("Failed to determine ODBC parameter count", count_result, statement, SQL_HANDLE_STMT);
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
                throw MakeDriverError<StatementError>(
                    "Cannot infer the type of a NULL ODBC parameter; add an explicit SQL cast or use a typed "
                    "std::optional where supported by the driver",
                    describe_result,
                    statement,
                    SQL_HANDLE_STMT
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
            throw MakeDriverError<StatementError>(
                fmt::format("Failed to bind ODBC parameter {}", index + 1),
                bind_result,
                statement,
                SQL_HANDLE_STMT
            );
        }
    }

    detail::CheckDeadlineNotExpired(deadline);
    const auto execute_result = SQLExecute(statement);
    detail::CheckDeadlineNotExpired(deadline);
    if (!SQL_SUCCEEDED(execute_result) && execute_result != SQL_NO_DATA) {
        throw MakeDriverError<
            StatementError>("Failed to execute prepared ODBC query", execute_result, statement, SQL_HANDLE_STMT);
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
        throw MakeDriverError<
            StatementError>("Failed to prepare ODBC query", prepare_result, statement, SQL_HANDLE_STMT);
    }
    BindParameters(statement, parameters, deadline);
    return SQL_SUCCESS;
}

}  // namespace

Connection::Connection(const std::string& dsn)
    : Connection{dsn, engine::current_task::GetBlockingTaskProcessor(), detail::GetExecuteDeadline(detail::kDefaultStatementTimeout)}
{}

Connection::Connection(const std::string& dsn, engine::Deadline deadline)
    : Connection{dsn, engine::current_task::GetBlockingTaskProcessor(), deadline}
{}

Connection::Connection(
    const std::string& dsn,
    engine::TaskProcessor& blocking_task_processor,
    engine::Deadline deadline
)
    : blocking_task_processor_{blocking_task_processor},
      env_{SQL_NULL_HENV, &DestroyEnvironmentHandle},
      handle_{SQL_NULL_HDBC, &DestroyDatabaseHandle}
{
    CheckOperationInterrupted(deadline);
    auto handles = RunBlocking(blocking_task_processor_, [dsn, deadline] {
        ConfigureDriverManager();
        auto environment = MakeEnvironmentHandle();
        auto result =
            SQLSetEnvAttr(environment.get(), SQL_ATTR_ODBC_VERSION, reinterpret_cast<SQLPOINTER>(SQL_OV_ODBC3), 0);
        if (!SQL_SUCCEEDED(result)) {
            throw MakeDriverError<
                ConnectionError>("Failed to set ODBC version", result, environment.get(), SQL_HANDLE_ENV);
        }

        auto database = MakeDatabaseHandle(environment.get());
        if (deadline.IsReachable()) {
            const auto time_left = deadline.TimeLeft();
            if (time_left <= engine::Deadline::Duration::zero()) {
                throw OperationInterrupted("Cancelled by deadline");
            }
            const auto timeout = std::chrono::ceil<std::chrono::seconds>(time_left);
            const auto timeout_seconds = static_cast<SQLULEN>(timeout.count());
            result = SQLSetConnectAttr(
                database.get(),
                SQL_ATTR_LOGIN_TIMEOUT,
                reinterpret_cast<SQLPOINTER>(static_cast<std::uintptr_t>(timeout_seconds)),
                SQL_IS_UINTEGER
            );
            if (!SQL_SUCCEEDED(result)) {
                throw MakeDriverError<
                    ConnectionError>("Failed to set ODBC login timeout", result, database.get(), SQL_HANDLE_DBC);
            }
        }

        std::vector<SQLCHAR> dsn_buffer(dsn.begin(), dsn.end());
        dsn_buffer.push_back('\0');
        result = SQLDriverConnect(
            database.get(),
            nullptr,
            dsn_buffer.data(),
            SQL_NTS,
            nullptr,
            0,
            nullptr,
            SQL_DRIVER_NOPROMPT
        );
        if (!SQL_SUCCEEDED(result)) {
            throw MakeDriverError<
                ConnectionError>("Failed to connect to database", result, database.get(), SQL_HANDLE_DBC);
        }
        if (deadline.IsReachable() && deadline.IsReached()) {
            throw OperationInterrupted("Cancelled by deadline");
        }

        auto driver_capabilities = detail::DriverCapabilities::Read(database.get(), deadline);
        return ConnectedHandles{
            std::move(environment),
            std::move(database),
            std::move(driver_capabilities),
        };
    });

    try {
        CheckOperationInterrupted(deadline);
    } catch (...) {
        DestroyConnectionHandlesOnBlockingTaskProcessor(
            blocking_task_processor_,
            handles.environment,
            handles.database
        );
        throw;
    }
    driver_capabilities_ = std::move(handles.driver_capabilities);
    env_ = std::move(handles.environment);
    handle_ = std::move(handles.database);
}

Connection::~Connection() { DestroyConnectionHandlesOnBlockingTaskProcessor(blocking_task_processor_, env_, handle_); }

const detail::DriverCapabilities& Connection::GetDriverCapabilities() const noexcept { return driver_capabilities_; }

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
    return Query(storages::odbc::Query{std::string{query}}, parameters, deadline);
}

ResultSet Connection::Query(
    const storages::odbc::Query& query,
    const impl::ParameterList& parameters,
    engine::Deadline deadline
) {
    CheckOperationInterrupted(deadline);

    const auto statement = query.GetStatementView();
    tracing::Span span{detail::tracing::MakeQuerySpanName(statement)};
    span.AddTag(tracing::kDatabaseType, "odbc");
    const auto span_tags = detail::tracing::MakeQuerySpanTags(query);
    if (span_tags.statement_name) {
        span.AddTag(tracing::kDatabaseStatementName, std::string{*span_tags.statement_name});
    }
    if (span_tags.statement) {
        span.AddTag(tracing::kDatabaseStatement, std::string{*span_tags.statement});
    }

    auto guard = GetBrokenGuard();
    try {
        return guard.Execute([&] {
            return RunBlockingChecked(
                blocking_task_processor_,
                deadline,
                [this, query = std::string{statement}, parameters, deadline] {
                    const std::lock_guard lock{handle_mutex_};
                    try {
                        auto stmt = MakeStatementHandle(handle_.get());

                        if (deadline.IsReachable()) {
                            const auto left = deadline.TimeLeft();
                            if (left <= engine::Deadline::Duration::zero()) {
                                throw OperationInterrupted("Cancelled by deadline");
                            }
                            const auto seconds = std::chrono::ceil<std::chrono::seconds>(left);
                            const auto timeout_sec = static_cast<SQLULEN>(seconds.count());
                            // SQL_ATTR_QUERY_TIMEOUT is in whole seconds; the
                            // exact deadline is checked in the caller afterwards.
                            const auto timeout_result = SQLSetStmtAttr(
                                stmt.get(),
                                SQL_ATTR_QUERY_TIMEOUT,
                                reinterpret_cast<SQLPOINTER>(static_cast<std::uintptr_t>(timeout_sec)),
                                0
                            );
                            if (!SQL_SUCCEEDED(timeout_result)) {
                                throw MakeDriverError<StatementError>(
                                    "Failed to set ODBC query timeout",
                                    timeout_result,
                                    stmt.get(),
                                    SQL_HANDLE_STMT
                                );
                            }
                        }

                        const auto result = ExecuteStatement(stmt.get(), query, parameters, deadline);
                        if (!SQL_SUCCEEDED(result) && result != SQL_NO_DATA) {
                            throw MakeDriverError<
                                StatementError>("Failed to execute query", result, stmt.get(), SQL_HANDLE_STMT);
                        }
                        return ResultSet(MaterializeResult(stmt.get(), deadline));
                    } catch (const StatementError& ex) {
                        if (ex.IsInvalidHandle() || detail::HasConnectionError(ex.GetDiagnostics())) {
                            NotifyBroken();
                        } else {
                            UpdateBrokenFromDriver();
                        }
                        throw;
                    }
                }
            );
        });
    } catch (const OperationInterrupted&) {
        // The state after a synchronous call that crossed a deadline or was
        // cancelled is uncertain. Never return this HDBC to the pool.
        NotifyBroken();
        throw;
    } catch (const Error& ex) {
        span.AddTag(tracing::kErrorFlag, true);
        span.AddTag(tracing::kErrorMessage, ex.what());
        throw;
    }
}

bool Connection::IsBroken() const { return broken_.load() || in_transaction_.load(); }

bool Connection::IsMarkedBroken() const noexcept { return broken_.load(); }

void Connection::NotifyBroken() { broken_.store(true); }

void Connection::UpdateBrokenFromDriver() noexcept {
    SQLUINTEGER state = SQL_CD_TRUE;
    const auto result = SQLGetConnectAttr(handle_.get(), SQL_ATTR_CONNECTION_DEAD, &state, sizeof(state), nullptr);
    if (!SQL_SUCCEEDED(result) || state == SQL_CD_TRUE) {
        NotifyBroken();
    }
}

detail::BrokenGuard Connection::GetBrokenGuard() { return detail::BrokenGuard{*this}; }

bool Connection::IsInsideTransaction() const noexcept { return in_transaction_.load(); }

void Connection::Begin(const TransactionOptions& options, engine::Deadline deadline) {
    if (IsInsideTransaction()) {
        throw ConnectionError("Cannot begin an ODBC transaction while another transaction is active");
    }

    const auto transaction_capability = driver_capabilities_.GetTransactionCapability();
    if (transaction_capability && *transaction_capability == detail::TransactionCapability::kNone) {
        throw TransactionException("ODBC driver reports that transactions are not supported");
    }
    if (options.isolation_level &&
        !detail::IsIsolationSupported(driver_capabilities_.GetTransactionIsolationOptions(), *options.isolation_level))
    {
        throw TransactionException(fmt::format(
            "ODBC driver does not report support for requested {} isolation",
            detail::ToStringView(*options.isolation_level)
        ));
    }

    try {
        RunBlockingChecked(blocking_task_processor_, deadline, [this, options, deadline] {
            const std::lock_guard lock{handle_mutex_};
            TransactionAttributes snapshot;
            if (options.isolation_level) {
                snapshot.isolation =
                    ReadConnectionAttribute(handle_.get(), SQL_ATTR_TXN_ISOLATION, "SQL_ATTR_TXN_ISOLATION", deadline);
            }
            if (options.access_mode) {
                snapshot.access_mode =
                    ReadConnectionAttribute(handle_.get(), SQL_ATTR_ACCESS_MODE, "SQL_ATTR_ACCESS_MODE", deadline);
            }
            snapshot.autocommit =
                ReadConnectionAttribute(handle_.get(), SQL_ATTR_AUTOCOMMIT, "SQL_ATTR_AUTOCOMMIT", deadline);

            TransactionAttributes attempted_restore;
            try {
                if (options.isolation_level) {
                    attempted_restore.isolation = snapshot.isolation;
                    SetConnectionAttributeVerified(
                        handle_.get(),
                        SQL_ATTR_TXN_ISOLATION,
                        detail::ToOdbcIsolation(*options.isolation_level),
                        "SQL_ATTR_TXN_ISOLATION",
                        deadline
                    );
                }
                if (options.access_mode) {
                    attempted_restore.access_mode = snapshot.access_mode;
                    const auto access_mode =
                        *options.access_mode == AccessMode::kReadOnly ? SQL_MODE_READ_ONLY : SQL_MODE_READ_WRITE;
                    SetConnectionAttributeVerified(
                        handle_.get(),
                        SQL_ATTR_ACCESS_MODE,
                        access_mode,
                        "SQL_ATTR_ACCESS_MODE",
                        deadline
                    );
                }

                attempted_restore.autocommit = snapshot.autocommit;
                SetConnectionAttributeVerified(
                    handle_.get(),
                    SQL_ATTR_AUTOCOMMIT,
                    SQL_AUTOCOMMIT_OFF,
                    "SQL_ATTR_AUTOCOMMIT",
                    deadline
                );
            } catch (...) {
                const auto begin_error = std::current_exception();
                if (attempted_restore.isolation || attempted_restore.access_mode || attempted_restore.autocommit) {
                    try {
                        RestoreTransactionAttributes(
                            attempted_restore,
                            engine::Deadline::FromDuration(detail::kDefaultCleanupTimeout)
                        );
                    } catch (const std::exception& ex) {
                        NotifyBroken();
                        LOG_ERROR() << "Failed to restore ODBC attributes after transaction begin failure: " << ex;
                    } catch (...) {
                        NotifyBroken();
                        LOG_ERROR() << "Failed to restore ODBC attributes after transaction begin failure";
                    }
                }
                std::rethrow_exception(begin_error);
            }

            transaction_attributes_snapshot_ = snapshot;
            in_transaction_.store(true);
        });
    } catch (const OperationInterrupted&) {
        CleanupInterruptedBegin();
        throw;
    } catch (const ConnectionError& ex) {
        if (ex.IsInvalidHandle() || ex.HasSqlStateClass("08")) {
            NotifyBroken();
        }
        throw;
    }
}

void Connection::Commit(engine::Deadline deadline) {
    auto guard = GetBrokenGuard();
    try {
        guard.Execute([this, deadline] {
            if (!IsInsideTransaction()) {
                throw ConnectionError("Cannot commit an ODBC connection outside a transaction");
            }
            RunBlockingChecked(blocking_task_processor_, deadline, [this, deadline] {
                const std::lock_guard lock{handle_mutex_};
                EndTransaction(SQL_COMMIT, "commit", deadline);
            });
        });
    } catch (const OperationInterrupted&) {
        NotifyBroken();
        throw;
    }
}

void Connection::Rollback(engine::Deadline deadline) {
    auto guard = GetBrokenGuard();
    try {
        guard.Execute([this, deadline] {
            if (!IsInsideTransaction()) {
                throw ConnectionError("Cannot roll back an ODBC connection outside a transaction");
            }
            RunBlockingChecked(blocking_task_processor_, deadline, [this, deadline] {
                const std::lock_guard lock{handle_mutex_};
                EndTransaction(SQL_ROLLBACK, "roll back", deadline);
            });
        });
    } catch (const OperationInterrupted&) {
        NotifyBroken();
        throw;
    }
}

void Connection::EndTransaction(SQLSMALLINT completion_type, std::string_view operation, engine::Deadline deadline) {
    if (!transaction_attributes_snapshot_) {
        throw ConnectionError("ODBC transaction attribute snapshot is missing");
    }

    detail::CheckDeadlineNotExpired(deadline);
    const auto result = SQLEndTran(SQL_HANDLE_DBC, handle_.get(), completion_type);
    std::exception_ptr deadline_error;
    try {
        detail::CheckDeadlineNotExpired(deadline);
    } catch (...) {
        deadline_error = std::current_exception();
    }
    auto diagnostics =
        result == SQL_SUCCESS
            ? std::vector<DiagnosticRecord>{}
            : detail::GetSQLDiagnostics(handle_.get(), SQL_HANDLE_DBC);
    if (!SQL_SUCCEEDED(result)) {
        throw MakeConnectionError(
            fmt::format("Failed to {} ODBC transaction", operation),
            result,
            std::move(diagnostics)
        );
    }
    if (result == SQL_SUCCESS_WITH_INFO) {
        LogConnectionAttributeWarnings(fmt::format("ODBC transaction {}", operation), diagnostics);
    }

    RestoreTransactionAttributes(
        *transaction_attributes_snapshot_,
        engine::Deadline::FromDuration(detail::kDefaultCleanupTimeout)
    );
    transaction_attributes_snapshot_.reset();
    in_transaction_.store(false);

    if (deadline_error) {
        std::rethrow_exception(deadline_error);
    }
}

void Connection::RestoreTransactionAttributes(const TransactionAttributes& attributes, engine::Deadline deadline) {
    std::exception_ptr first_error;
    const auto restore = [&](SQLINTEGER attribute, const std::optional<SQLUINTEGER>& value, std::string_view name) {
        if (!value) {
            return;
        }
        try {
            SetConnectionAttributeVerified(handle_.get(), attribute, *value, name, deadline);
        } catch (...) {
            if (!first_error) {
                first_error = std::current_exception();
            }
        }
    };

    // SQLEndTran leaves manual-commit mode active but no transaction open, so
    // isolation and access mode are legal to restore before autocommit.
    restore(SQL_ATTR_TXN_ISOLATION, attributes.isolation, "SQL_ATTR_TXN_ISOLATION");
    restore(SQL_ATTR_ACCESS_MODE, attributes.access_mode, "SQL_ATTR_ACCESS_MODE");
    restore(SQL_ATTR_AUTOCOMMIT, attributes.autocommit, "SQL_ATTR_AUTOCOMMIT");

    if (first_error) {
        std::rethrow_exception(first_error);
    }
}

void Connection::CleanupInterruptedBegin() noexcept {
    if (!IsInsideTransaction()) {
        return;
    }

    try {
        RunBlocking(blocking_task_processor_, [this] {
            const std::lock_guard lock{handle_mutex_};
            if (IsInsideTransaction()) {
                EndTransaction(
                    SQL_ROLLBACK,
                    "roll back interrupted begin",
                    engine::Deadline::FromDuration(detail::kDefaultCleanupTimeout)
                );
            }
        });
    } catch (const std::exception& ex) {
        NotifyBroken();
        LOG_ERROR() << "Failed to clean up interrupted ODBC transaction begin: " << ex;
    } catch (...) {
        NotifyBroken();
        LOG_ERROR() << "Failed to clean up interrupted ODBC transaction begin";
    }
}

}  // namespace storages::odbc

USERVER_NAMESPACE_END
