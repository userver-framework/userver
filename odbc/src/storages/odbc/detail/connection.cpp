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
#include <stdexcept>
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
#include <storages/odbc/detail/command_control_store.hpp>
#include <storages/odbc/detail/deadline.hpp>
#include <storages/odbc/detail/diag_wrapper.hpp>
#include <storages/odbc/detail/result_chunk.hpp>
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

template <typename Exception>
Exception MakeDiagnosticError(std::string message, std::vector<DiagnosticRecord> diagnostics) {
    const auto formatted = detail::FormatSQLDiagnostics(diagnostics);
    if (!formatted.empty()) {
        message += ": ";
        message += formatted;
    }
    return Exception{std::move(message), std::move(diagnostics)};
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

void LogOdbcWarnings(std::string_view operation, const std::vector<DiagnosticRecord>& diagnostics) {
    constexpr std::size_t kMaxWarningLength = 1024;
    auto formatted = detail::FormatSQLDiagnostics(diagnostics);
    if (formatted.size() > kMaxWarningLength) {
        formatted.resize(kMaxWarningLength);
        formatted += "...";
    }
    LOG_WARNING()
        << operation << " completed with warning: " << (formatted.empty() ? "no diagnostic records" : formatted);
}

void LogOdbcWarnings(std::string_view operation, SQLHANDLE handle, SQLSMALLINT handle_type) {
    LogOdbcWarnings(operation, detail::GetSQLDiagnostics(handle, handle_type));
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

bool IsTruncationWarning(const std::vector<DiagnosticRecord>& diagnostics);
bool HasNonTruncationWarning(const std::vector<DiagnosticRecord>& diagnostics);
bool HasDataLossWarning(const std::vector<DiagnosticRecord>& diagnostics);
std::vector<DiagnosticRecord> GetWarnings(SQLRETURN result, SQLHANDLE handle, SQLSMALLINT handle_type);

void HandleStatementWarnings(SQLRETURN result, SQLHSTMT statement, std::string_view operation, bool reject_data_loss);

detail::ResultWrapper::Column DescribeColumn(SQLHSTMT statement, SQLUSMALLINT column, engine::Deadline deadline) {
    std::array<SQLCHAR, 256> buffer{};
    SQLSMALLINT name_length = 0;
    SQLSMALLINT type = SQL_UNKNOWN_TYPE;
    SQLULEN size = 0;
    SQLSMALLINT decimal_digits = 0;
    detail::CheckDeadlineNotExpired(deadline);
    auto result = SQLDescribeCol(
        statement,
        column,
        buffer.data(),
        static_cast<SQLSMALLINT>(buffer.size()),
        &name_length,
        &type,
        &size,
        &decimal_digits,
        nullptr
    );
    auto warnings = GetWarnings(result, statement, SQL_HANDLE_STMT);
    detail::CheckDeadlineNotExpired(deadline);
    CheckStatementResult(result, statement, "describe result column");

    if (name_length >= static_cast<SQLSMALLINT>(buffer.size())) {
        if (result == SQL_SUCCESS_WITH_INFO && HasNonTruncationWarning(warnings)) {
            LogOdbcWarnings("Describing ODBC result column", warnings);
        }
        std::vector<SQLCHAR> long_buffer(static_cast<std::size_t>(name_length) + 1);
        detail::CheckDeadlineNotExpired(deadline);
        result = SQLDescribeCol(
            statement,
            column,
            long_buffer.data(),
            static_cast<SQLSMALLINT>(long_buffer.size()),
            &name_length,
            &type,
            &size,
            &decimal_digits,
            nullptr
        );
        warnings = GetWarnings(result, statement, SQL_HANDLE_STMT);
        detail::CheckDeadlineNotExpired(deadline);
        CheckStatementResult(result, statement, "describe result column");
        if (result == SQL_SUCCESS_WITH_INFO) {
            if (HasDataLossWarning(warnings)) {
                throw MakeDiagnosticError<StatementError>(
                    "ODBC driver truncated a result column description after an exact-size retry",
                    std::move(warnings)
                );
            }
            LogOdbcWarnings("Describing ODBC result column after an exact-size retry", warnings);
        }
        return {
            std::string{reinterpret_cast<const char*>(long_buffer.data()), static_cast<std::size_t>(name_length)},
            type,
            size,
            decimal_digits,
        };
    }

    if (result == SQL_SUCCESS_WITH_INFO) {
        if (HasDataLossWarning(warnings)) {
            throw MakeDiagnosticError<StatementError>(
                "ODBC driver truncated a result column description without reporting a larger name",
                std::move(warnings)
            );
        }
        LogOdbcWarnings("Describing ODBC result column", warnings);
    }

    return {
        std::string{reinterpret_cast<const char*>(buffer.data()), static_cast<std::size_t>(name_length)},
        type,
        size,
        decimal_digits,
    };
}

bool IsTruncationWarning(const std::vector<DiagnosticRecord>& diagnostics) {
    return std::any_of(diagnostics.begin(), diagnostics.end(), [](const DiagnosticRecord& diagnostic) {
        return diagnostic.sql_state == "01004";
    });
}

bool HasNonTruncationWarning(const std::vector<DiagnosticRecord>& diagnostics) {
    return std::any_of(diagnostics.begin(), diagnostics.end(), [](const DiagnosticRecord& diagnostic) {
        return diagnostic.sql_state != "01004";
    });
}

bool HasDataLossWarning(const std::vector<DiagnosticRecord>& diagnostics) {
    return std::any_of(diagnostics.begin(), diagnostics.end(), [](const DiagnosticRecord& diagnostic) {
        return diagnostic.sql_state == "01004" || diagnostic.sql_state == "01S07" || diagnostic.sql_state == "22003";
    });
}

std::vector<DiagnosticRecord> GetWarnings(SQLRETURN result, SQLHANDLE handle, SQLSMALLINT handle_type) {
    return result == SQL_SUCCESS_WITH_INFO
               ? detail::GetSQLDiagnostics(handle, handle_type)
               : std::vector<DiagnosticRecord>{};
}

void HandleStatementWarnings(SQLRETURN result, SQLHSTMT statement, std::string_view operation, bool reject_data_loss) {
    if (result != SQL_SUCCESS_WITH_INFO) {
        return;
    }
    auto diagnostics = detail::GetSQLDiagnostics(statement, SQL_HANDLE_STMT);
    if (reject_data_loss && HasDataLossWarning(diagnostics)) {
        throw MakeDiagnosticError<
            StatementError>(fmt::format("ODBC driver lost information while {}", operation), std::move(diagnostics));
    }
    LogOdbcWarnings(fmt::format("ODBC statement {}", operation), diagnostics);
}

detail::ResultWrapper::Cell ReadTextCell(SQLHSTMT statement, SQLUSMALLINT column, engine::Deadline deadline) {
    constexpr std::size_t kChunkSize = 4096;
    std::array<SQLCHAR, kChunkSize> buffer{};
    std::string value;
    std::optional<SQLLEN> previous_remaining;

    while (true) {
        SQLLEN indicator = 0;
        detail::CheckDeadlineNotExpired(deadline);
        const auto result =
            SQLGetData(statement, column, SQL_C_CHAR, buffer.data(), static_cast<SQLLEN>(buffer.size()), &indicator);
        const auto warnings = GetWarnings(result, statement, SQL_HANDLE_STMT);
        detail::CheckDeadlineNotExpired(deadline);

        if (result == SQL_NO_DATA) {
            break;
        }
        CheckStatementResult(result, statement, "read result column");
        if (indicator == SQL_NULL_DATA) {
            return {std::nullopt};
        }
        const bool truncated = IsTruncationWarning(warnings);
        const auto
            chunk = detail::AccountResultChunk(result, indicator, truncated, buffer.size() - 1, previous_remaining);
        value.append(reinterpret_cast<const char*>(buffer.data()), chunk.size);
        previous_remaining = chunk.known_remaining;
        if (chunk.has_more) {
            if (HasNonTruncationWarning(warnings)) {
                LogOdbcWarnings("Reading ODBC character result", warnings);
            }
            continue;
        }
        if (result == SQL_SUCCESS_WITH_INFO) {
            LogOdbcWarnings("Reading ODBC character result", warnings);
        }
        if (result == SQL_SUCCESS || result == SQL_SUCCESS_WITH_INFO) {
            break;
        }
    }

    return {detail::ResultWrapper::Cell::Value{std::move(value)}};
}

detail::ResultWrapper::Cell ReadBytesCell(SQLHSTMT statement, SQLUSMALLINT column, engine::Deadline deadline) {
    constexpr std::size_t kChunkSize = 4096;
    std::array<SQLCHAR, kChunkSize> buffer{};
    Bytes::Container value;
    std::optional<SQLLEN> previous_remaining;

    while (true) {
        SQLLEN indicator = 0;
        detail::CheckDeadlineNotExpired(deadline);
        const auto result =
            SQLGetData(statement, column, SQL_C_BINARY, buffer.data(), static_cast<SQLLEN>(buffer.size()), &indicator);
        const auto warnings = GetWarnings(result, statement, SQL_HANDLE_STMT);
        detail::CheckDeadlineNotExpired(deadline);

        if (result == SQL_NO_DATA) {
            break;
        }
        CheckStatementResult(result, statement, "read binary result column");
        if (indicator == SQL_NULL_DATA) {
            return {std::nullopt};
        }
        const bool truncated = IsTruncationWarning(warnings);
        const auto chunk = detail::AccountResultChunk(result, indicator, truncated, buffer.size(), previous_remaining);
        value.insert(value.end(), buffer.begin(), buffer.begin() + static_cast<std::ptrdiff_t>(chunk.size));
        previous_remaining = chunk.known_remaining;
        if (chunk.has_more) {
            if (HasNonTruncationWarning(warnings)) {
                LogOdbcWarnings("Reading ODBC binary result", warnings);
            }
            continue;
        }
        if (result == SQL_SUCCESS_WITH_INFO) {
            LogOdbcWarnings("Reading ODBC binary result", warnings);
        }
        if (result == SQL_SUCCESS || result == SQL_SUCCESS_WITH_INFO) {
            break;
        }
    }

    return {detail::ResultWrapper::Cell::Value{Bytes{std::move(value)}}};
}

template <typename Struct, typename ValueFactory>
detail::ResultWrapper::Cell ReadFixedCell(
    SQLHSTMT statement,
    SQLUSMALLINT column,
    SQLSMALLINT c_type,
    std::string_view type_name,
    engine::Deadline deadline,
    ValueFactory&& value_factory
) {
    Struct value{};
    SQLLEN indicator = 0;
    detail::CheckDeadlineNotExpired(deadline);
    const auto result = SQLGetData(statement, column, c_type, &value, static_cast<SQLLEN>(sizeof(value)), &indicator);
    auto warnings = GetWarnings(result, statement, SQL_HANDLE_STMT);
    detail::CheckDeadlineNotExpired(deadline);
    CheckStatementResult(result, statement, fmt::format("read {} result column", type_name));
    if (indicator == SQL_NULL_DATA) {
        return {std::nullopt};
    }
    detail::ValidateFixedResultSize(indicator, sizeof(value));
    if (result == SQL_SUCCESS_WITH_INFO) {
        if (HasDataLossWarning(warnings)) {
            throw MakeDiagnosticError<ResultSetError>(
                fmt::format("ODBC driver lost information while converting a fixed-size {} result", type_name),
                std::move(warnings)
            );
        }
        LogOdbcWarnings(fmt::format("Reading ODBC {} result", type_name), warnings);
    }
    try {
        return {detail::ResultWrapper::Cell::Value{std::forward<ValueFactory>(value_factory)(value)}};
    } catch (const std::invalid_argument& ex) {
        throw ResultSetError(fmt::format("ODBC driver returned an invalid {} value: {}", type_name, ex.what()));
    }
}

std::string DecodeNumericMagnitude(const SQL_NUMERIC_STRUCT& value) {
    std::vector<std::uint8_t> decimal_digits{0};
    for (std::size_t byte_index = sizeof(value.val); byte_index > 0; --byte_index) {
        unsigned carry = value.val[byte_index - 1];
        for (auto& digit : decimal_digits) {
            const auto next = static_cast<unsigned>(digit) * 256U + carry;
            digit = static_cast<std::uint8_t>(next % 10U);
            carry = next / 10U;
        }
        while (carry != 0) {
            decimal_digits.push_back(static_cast<std::uint8_t>(carry % 10U));
            carry /= 10U;
        }
    }
    while (decimal_digits.size() > 1 && decimal_digits.back() == 0) {
        decimal_digits.pop_back();
    }
    std::string result;
    result.reserve(decimal_digits.size());
    for (auto iterator = decimal_digits.rbegin(); iterator != decimal_digits.rend(); ++iterator) {
        result.push_back(static_cast<char>('0' + *iterator));
    }
    return result;
}

detail::ResultWrapper::Cell ReadDecimalCell(
    SQLHSTMT statement,
    SQLUSMALLINT column,
    const detail::ResultWrapper::Column& metadata,
    engine::Deadline deadline
) {
    if (metadata.size == 0 || metadata.size > 38 || metadata.decimal_digits < 0 ||
        static_cast<SQLULEN>(metadata.decimal_digits) > metadata.size)
    {
        throw ResultSetError(fmt::format(
            "ODBC Decimal column {} has invalid or unknown precision/scale metadata: precision {}, scale {}",
            column,
            metadata.size,
            metadata.decimal_digits
        ));
    }

    SQLHDESC descriptor = SQL_NULL_HDESC;
    detail::CheckDeadlineNotExpired(deadline);
    const auto descriptor_result = SQLGetStmtAttr(
        statement,
        SQL_ATTR_APP_ROW_DESC,
        &descriptor,
        static_cast<SQLINTEGER>(sizeof(descriptor)),
        nullptr
    );
    detail::CheckDeadlineNotExpired(deadline);
    if (!SQL_SUCCEEDED(descriptor_result)) {
        throw MakeDriverError<StatementError>(
            "Failed to obtain the descriptor for an ODBC Decimal result",
            descriptor_result,
            statement,
            SQL_HANDLE_STMT
        );
    }
    if (descriptor_result == SQL_SUCCESS_WITH_INFO) {
        LogOdbcWarnings("Obtaining the descriptor for an ODBC Decimal result", statement, SQL_HANDLE_STMT);
    }

    const auto set_field = [&](SQLSMALLINT field, SQLLEN value, std::string_view name) {
        detail::CheckDeadlineNotExpired(deadline);
        const auto result = SQLSetDescField(
            descriptor,
            static_cast<SQLSMALLINT>(column),
            field,
            reinterpret_cast<SQLPOINTER>(static_cast<std::uintptr_t>(value)),
            SQL_IS_INTEGER
        );
        detail::CheckDeadlineNotExpired(deadline);
        if (!SQL_SUCCEEDED(result)) {
            throw MakeDriverError<StatementError>(
                fmt::format("Failed to set {} for an ODBC Decimal result", name),
                result,
                descriptor,
                SQL_HANDLE_DESC
            );
        }
        if (result == SQL_SUCCESS_WITH_INFO) {
            LogOdbcWarnings(fmt::format("Setting {} for an ODBC Decimal result", name), descriptor, SQL_HANDLE_DESC);
        }
    };
    set_field(SQL_DESC_CONCISE_TYPE, SQL_C_NUMERIC, "C type");
    set_field(SQL_DESC_PRECISION, static_cast<SQLLEN>(metadata.size), "precision");
    set_field(SQL_DESC_SCALE, metadata.decimal_digits, "scale");

    SQL_NUMERIC_STRUCT value{};
    SQLLEN indicator = 0;
    detail::CheckDeadlineNotExpired(deadline);
    const auto
        result = SQLGetData(statement, column, SQL_ARD_TYPE, &value, static_cast<SQLLEN>(sizeof(value)), &indicator);
    auto warnings = GetWarnings(result, statement, SQL_HANDLE_STMT);
    detail::CheckDeadlineNotExpired(deadline);
    CheckStatementResult(result, statement, "read Decimal result column");
    if (indicator == SQL_NULL_DATA) {
        return {std::nullopt};
    }
    detail::ValidateFixedResultSize(indicator, sizeof(value));
    if (result == SQL_SUCCESS_WITH_INFO) {
        if (HasDataLossWarning(warnings)) {
            throw MakeDiagnosticError<
                ResultSetError>("ODBC driver lost information while converting a Decimal result", std::move(warnings));
        }
        LogOdbcWarnings("Reading ODBC Decimal result", warnings);
    }
    if (value.sign != 0 && value.sign != 1) {
        throw ResultSetError("ODBC driver returned an invalid SQL_NUMERIC_STRUCT sign");
    }
    if (value.precision == 0 || value.precision > metadata.size || value.scale != metadata.decimal_digits) {
        throw ResultSetError(fmt::format(
            "ODBC driver returned invalid Decimal value precision/scale {},{} for column precision/scale {},{}",
            value.precision,
            value.scale,
            metadata.size,
            metadata.decimal_digits
        ));
    }

    auto representation = DecodeNumericMagnitude(value);
    const bool is_zero = representation == "0";
    const auto scale = static_cast<std::size_t>(metadata.decimal_digits);
    detail::ValidateNumericMagnitude(representation.size(), metadata.size, scale);
    if (scale != 0) {
        if (representation.size() <= scale) {
            representation.insert(0, scale + 1 - representation.size(), '0');
        }
        representation.insert(representation.size() - scale, 1, '.');
    }
    if (value.sign == 0 && !is_zero) {
        representation.insert(representation.begin(), '-');
    }
    return {detail::ResultWrapper::Cell::Value{detail::ResultWrapper::DecimalValue{
        std::move(representation),
        static_cast<std::uint8_t>(metadata.size),
        static_cast<std::uint8_t>(metadata.decimal_digits),
    }}};
}

detail::ResultWrapper::Cell ReadCell(
    SQLHSTMT statement,
    SQLUSMALLINT column,
    const detail::ResultWrapper::Column& metadata,
    engine::Deadline deadline
) {
    if (metadata.type == SQL_TYPE_DATE || metadata.type == SQL_DATE) {
        return ReadFixedCell<
            SQL_DATE_STRUCT>(statement, column, SQL_C_TYPE_DATE, "Date", deadline, [](const SQL_DATE_STRUCT& value) {
            return Date{
                static_cast<std::uint32_t>(value.year),
                static_cast<std::uint32_t>(value.month),
                static_cast<std::uint32_t>(value.day),
            };
        });
    }
    if (metadata.type == SQL_TYPE_TIME || metadata.type == SQL_TIME) {
        return ReadFixedCell<
            SQL_TIME_STRUCT>(statement, column, SQL_C_TYPE_TIME, "Time", deadline, [](const SQL_TIME_STRUCT& value) {
            return Time{
                static_cast<std::uint32_t>(value.hour),
                static_cast<std::uint32_t>(value.minute),
                static_cast<std::uint32_t>(value.second),
            };
        });
    }
    if (metadata.type == SQL_TYPE_TIMESTAMP || metadata.type == SQL_TIMESTAMP) {
        return ReadFixedCell<SQL_TIMESTAMP_STRUCT>(
            statement,
            column,
            SQL_C_TYPE_TIMESTAMP,
            "Timestamp",
            deadline,
            [](const SQL_TIMESTAMP_STRUCT& value) {
                return Timestamp{
                    static_cast<std::uint32_t>(value.year),
                    value.month,
                    value.day,
                    value.hour,
                    value.minute,
                    value.second,
                    value.fraction,
                };
            }
        );
    }
    switch (metadata.type) {
        case SQL_BINARY:
        case SQL_VARBINARY:
        case SQL_LONGVARBINARY:
            return ReadBytesCell(statement, column, deadline);
        case SQL_DECIMAL:
        case SQL_NUMERIC:
            return ReadDecimalCell(statement, column, metadata, deadline);
        default:
            return ReadTextCell(statement, column, deadline);
    }
}

std::shared_ptr<detail::ResultWrapper> MaterializeResult(SQLHSTMT statement, engine::Deadline deadline) {
    SQLSMALLINT column_count = 0;
    detail::CheckDeadlineNotExpired(deadline);
    const auto column_count_result = SQLNumResultCols(statement, &column_count);
    HandleStatementWarnings(column_count_result, statement, "determining result column count", false);
    detail::CheckDeadlineNotExpired(deadline);
    CheckStatementResult(column_count_result, statement, "get result column count for");

    std::size_t rows_affected = 0;
    if (column_count == 0) {
        SQLLEN affected = 0;
        detail::CheckDeadlineNotExpired(deadline);
        const auto row_count_result = SQLRowCount(statement, &affected);
        HandleStatementWarnings(row_count_result, statement, "determining the affected row count", false);
        detail::CheckDeadlineNotExpired(deadline);
        rows_affected = SQL_SUCCEEDED(row_count_result) && affected > 0 ? static_cast<std::size_t>(affected) : 0;
    }

    std::vector<detail::ResultWrapper::Column> columns;
    columns.reserve(static_cast<std::size_t>(column_count));
    for (SQLSMALLINT index = 0; index < column_count; ++index) {
        columns.push_back(DescribeColumn(statement, static_cast<SQLUSMALLINT>(index + 1), deadline));
    }

    std::vector<detail::ResultWrapper::Row> rows;
    if (column_count > 0) {
        while (true) {
            detail::CheckDeadlineNotExpired(deadline);
            const auto fetch_result = SQLFetch(statement);
            HandleStatementWarnings(fetch_result, statement, "fetching a result row", false);
            detail::CheckDeadlineNotExpired(deadline);
            if (fetch_result == SQL_NO_DATA) {
                break;
            }
            CheckStatementResult(fetch_result, statement, "fetch row from");

            detail::ResultWrapper::Row row;
            row.reserve(static_cast<std::size_t>(column_count));
            for (SQLSMALLINT index = 0; index < column_count; ++index) {
                row.push_back(ReadCell(
                    statement,
                    static_cast<SQLUSMALLINT>(index + 1),
                    columns[static_cast<std::size_t>(index)],
                    deadline
                ));
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
    SQLSMALLINT decimal_digits;
    SQLPOINTER data;
    SQLLEN buffer_size;
};

struct BoundBytes final {
    explicit BoundBytes(const Bytes& value)
        : bytes{value.GetBytes().begin(), value.GetBytes().end()}
    {}

    SQLPOINTER Data() noexcept { return bytes.empty() ? &empty_value : bytes.data(); }

    std::vector<SQLCHAR> bytes;
    SQLCHAR empty_value{0};
};

SQL_NUMERIC_STRUCT MakeNumericStruct(const impl::DecimalParameter& parameter) {
    SQL_NUMERIC_STRUCT result{};
    result.precision = parameter.precision;
    result.scale = static_cast<SQLSCHAR>(parameter.scale);
    result.sign = parameter.representation.front() == '-' ? 0 : 1;

    for (const char ch : parameter.representation) {
        if (ch == '-' || ch == '+' || ch == '.') {
            continue;
        }
        unsigned carry = static_cast<unsigned>(ch - '0');
        for (auto& byte : result.val) {
            const auto value = static_cast<unsigned>(byte) * 10U + carry;
            byte = static_cast<SQLCHAR>(value & 0xffU);
            carry = value >> 8U;
        }
        if (carry != 0) {
            throw StatementError("ODBC Decimal magnitude exceeds SQL_NUMERIC_STRUCT capacity");
        }
    }
    return result;
}

struct BoundParameter final {
    using Value = std::variant<
        SQLCHAR,
        SQLBIGINT,
        SQLDOUBLE,
        std::string,
        BoundBytes,
        SQL_DATE_STRUCT,
        SQL_TIME_STRUCT,
        SQL_TIMESTAMP_STRUCT,
        SQL_NUMERIC_STRUCT>;

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
            case ParameterType::kBytes:
                return BoundBytes{parameter.Get<Bytes>()};
            case ParameterType::kDate: {
                const auto& date = parameter.Get<Date>();
                return SQL_DATE_STRUCT{
                    static_cast<SQLSMALLINT>(date.GetYear()),
                    static_cast<SQLUSMALLINT>(date.GetMonth()),
                    static_cast<SQLUSMALLINT>(date.GetDay()),
                };
            }
            case ParameterType::kTime: {
                const auto& time = parameter.Get<Time>();
                return SQL_TIME_STRUCT{
                    static_cast<SQLUSMALLINT>(time.GetHour()),
                    static_cast<SQLUSMALLINT>(time.GetMinute()),
                    static_cast<SQLUSMALLINT>(time.GetSecond()),
                };
            }
            case ParameterType::kTimestamp: {
                const auto& timestamp = parameter.Get<Timestamp>();
                const auto& date = timestamp.GetDate();
                const auto& time = timestamp.GetTime();
                return SQL_TIMESTAMP_STRUCT{
                    static_cast<SQLSMALLINT>(date.GetYear()),
                    static_cast<SQLUSMALLINT>(date.GetMonth()),
                    static_cast<SQLUSMALLINT>(date.GetDay()),
                    static_cast<SQLUSMALLINT>(time.GetHour()),
                    static_cast<SQLUSMALLINT>(time.GetMinute()),
                    static_cast<SQLUSMALLINT>(time.GetSecond()),
                    static_cast<SQLUINTEGER>(timestamp.GetFractionNanoseconds()),
                };
            }
            case ParameterType::kDecimal:
                return MakeNumericStruct(parameter.Get<impl::DecimalParameter>());
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
                0,
                &std::get<SQLCHAR>(parameter.value),
                static_cast<SQLLEN>(sizeof(SQLCHAR)),
            };
        case ParameterType::kSignedInteger:
            return {
                SQL_C_SBIGINT,
                SQL_BIGINT,
                19,
                0,
                &std::get<SQLBIGINT>(parameter.value),
                static_cast<SQLLEN>(sizeof(SQLBIGINT)),
            };
        case ParameterType::kUnsignedInteger:
            return {
                SQL_C_SBIGINT,
                SQL_BIGINT,
                19,
                0,
                &std::get<SQLBIGINT>(parameter.value),
                static_cast<SQLLEN>(sizeof(SQLBIGINT)),
            };
        case ParameterType::kFloatingPoint:
            return {
                SQL_C_DOUBLE,
                SQL_DOUBLE,
                15,
                0,
                &std::get<SQLDOUBLE>(parameter.value),
                static_cast<SQLLEN>(sizeof(SQLDOUBLE)),
            };
        case ParameterType::kString: {
            auto& string = std::get<std::string>(parameter.value);
            return {
                SQL_C_CHAR,
                SQL_VARCHAR,
                std::max<SQLULEN>(1, static_cast<SQLULEN>(string.size())),
                0,
                string.data(),
                static_cast<SQLLEN>(string.size()),
            };
        }
        case ParameterType::kBytes: {
            auto& bytes = std::get<BoundBytes>(parameter.value);
            return {
                SQL_C_BINARY,
                SQL_LONGVARBINARY,
                std::max<SQLULEN>(1, static_cast<SQLULEN>(bytes.bytes.size())),
                0,
                bytes.Data(),
                static_cast<SQLLEN>(bytes.bytes.size()),
            };
        }
        case ParameterType::kDate:
            return {
                SQL_C_TYPE_DATE,
                SQL_TYPE_DATE,
                10,
                0,
                &std::get<SQL_DATE_STRUCT>(parameter.value),
                static_cast<SQLLEN>(sizeof(SQL_DATE_STRUCT)),
            };
        case ParameterType::kTime: {
            return {
                SQL_C_TYPE_TIME,
                SQL_TYPE_TIME,
                8,
                0,
                &std::get<SQL_TIME_STRUCT>(parameter.value),
                static_cast<SQLLEN>(sizeof(SQL_TIME_STRUCT)),
            };
        }
        case ParameterType::kTimestamp: {
            auto& timestamp = std::get<SQL_TIMESTAMP_STRUCT>(parameter.value);
            return {
                SQL_C_TYPE_TIMESTAMP,
                SQL_TYPE_TIMESTAMP,
                static_cast<SQLULEN>(timestamp.fraction == 0 ? 19 : 29),
                static_cast<SQLSMALLINT>(timestamp.fraction == 0 ? 0 : 9),
                &timestamp,
                static_cast<SQLLEN>(sizeof(timestamp)),
            };
        }
        case ParameterType::kDecimal: {
            auto& decimal = std::get<SQL_NUMERIC_STRUCT>(parameter.value);
            return {
                SQL_C_NUMERIC,
                SQL_DECIMAL,
                decimal.precision,
                decimal.scale,
                &decimal,
                static_cast<SQLLEN>(sizeof(decimal)),
            };
        }
        case ParameterType::kUnknown:
            // SQLDescribeParam below replaces the SQL type. A dummy character
            // buffer keeps drivers that validate ValuePtr happy for NULL.
            return {
                SQL_C_CHAR,
                SQL_VARCHAR,
                1,
                0,
                std::get<std::string>(parameter.value).data(),
                0,
            };
    }
    UINVARIANT(false, "Unknown ODBC parameter type");
}

void BindParameters(SQLHSTMT statement, const impl::ParameterList& parameters, engine::Deadline deadline) {
    SQLSMALLINT expected_count = 0;
    detail::CheckDeadlineNotExpired(deadline);
    const auto count_result = SQLNumParams(statement, &expected_count);
    HandleStatementWarnings(count_result, statement, "determining parameter count", false);
    detail::CheckDeadlineNotExpired(deadline);
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
            detail::CheckDeadlineNotExpired(deadline);
            const auto describe_result = SQLDescribeParam(
                statement,
                static_cast<SQLUSMALLINT>(index + 1),
                &binding.sql_type,
                &binding.column_size,
                &decimal_digits,
                &nullable
            );
            HandleStatementWarnings(describe_result, statement, "describing an untyped NULL parameter", false);
            detail::CheckDeadlineNotExpired(deadline);
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
        const auto bind_buffer_size = parameter.type == impl::ParameterType::kDecimal ? SQLLEN{0} : binding.buffer_size;
        detail::CheckDeadlineNotExpired(deadline);
        const auto bind_result = SQLBindParameter(
            statement,
            static_cast<SQLUSMALLINT>(index + 1),
            SQL_PARAM_INPUT,
            binding.c_type,
            binding.sql_type,
            binding.column_size,
            binding.decimal_digits,
            binding.data,
            bind_buffer_size,
            &indicators[index]
        );
        HandleStatementWarnings(bind_result, statement, "binding a query parameter", false);
        detail::CheckDeadlineNotExpired(deadline);
        if (!SQL_SUCCEEDED(bind_result)) {
            throw MakeDriverError<StatementError>(
                fmt::format("Failed to bind ODBC parameter {}", index + 1),
                bind_result,
                statement,
                SQL_HANDLE_STMT
            );
        }
        if (parameter.type == impl::ParameterType::kDecimal) {
            SQLHDESC descriptor = SQL_NULL_HDESC;
            detail::CheckDeadlineNotExpired(deadline);
            const auto descriptor_result = SQLGetStmtAttr(
                statement,
                SQL_ATTR_APP_PARAM_DESC,
                &descriptor,
                static_cast<SQLINTEGER>(sizeof(descriptor)),
                nullptr
            );
            detail::CheckDeadlineNotExpired(deadline);
            if (!SQL_SUCCEEDED(descriptor_result)) {
                throw MakeDriverError<StatementError>(
                    fmt::format("Failed to obtain the descriptor for ODBC Decimal parameter {}", index + 1),
                    descriptor_result,
                    statement,
                    SQL_HANDLE_STMT
                );
            }
            if (descriptor_result == SQL_SUCCESS_WITH_INFO) {
                LogOdbcWarnings(
                    fmt::format("Obtaining the descriptor for ODBC Decimal parameter {}", index + 1),
                    statement,
                    SQL_HANDLE_STMT
                );
            }

            const auto record = static_cast<SQLSMALLINT>(index + 1);
            const auto set_field = [&](SQLSMALLINT field, SQLPOINTER value, std::string_view name) {
                detail::CheckDeadlineNotExpired(deadline);
                const auto result = SQLSetDescField(descriptor, record, field, value, 0);
                const auto warnings = GetWarnings(result, descriptor, SQL_HANDLE_DESC);
                detail::CheckDeadlineNotExpired(deadline);
                if (!SQL_SUCCEEDED(result)) {
                    throw MakeDriverError<StatementError>(
                        fmt::format("Failed to set {} for ODBC Decimal parameter {}", name, index + 1),
                        result,
                        descriptor,
                        SQL_HANDLE_DESC
                    );
                }
                if (result == SQL_SUCCESS_WITH_INFO) {
                    LogOdbcWarnings(fmt::format("Setting {} for ODBC Decimal parameter {}", name, index + 1), warnings);
                }
            };
            const auto numeric_field = [](SQLLEN value) {
                return reinterpret_cast<SQLPOINTER>(static_cast<std::uintptr_t>(value));
            };
            // Per ODBC descriptor sequencing rules, changing non-deferred APD
            // fields clears SQL_DESC_DATA_PTR. Restore it last so SQLExecute
            // sees the bound SQL_NUMERIC_STRUCT rather than a NULL parameter.
            set_field(SQL_DESC_TYPE, numeric_field(SQL_C_NUMERIC), "C type");
            set_field(SQL_DESC_PRECISION, numeric_field(binding.column_size), "precision");
            set_field(SQL_DESC_SCALE, numeric_field(binding.decimal_digits), "scale");
            set_field(SQL_DESC_DATA_PTR, binding.data, "data pointer");
        }
    }

    detail::CheckDeadlineNotExpired(deadline);
    const auto execute_result = SQLExecute(statement);
    HandleStatementWarnings(execute_result, statement, "executing a prepared query", true);
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
        HandleStatementWarnings(execute_result, statement, "executing a direct query", true);
        detail::CheckDeadlineNotExpired(deadline);
        return execute_result;
    }

    detail::CheckDeadlineNotExpired(deadline);
    const auto prepare_result = SQLPrepare(statement, query_buffer.data(), SQL_NTS);
    HandleStatementWarnings(prepare_result, statement, "preparing a query", false);
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

void Connection::SetCommandControlStore(std::shared_ptr<detail::CommandControlStore> store) {
    command_control_store_ = std::move(store);
}

CommandControl Connection::ResolveTransactionCommandControl(
    CommandControl transaction_base,
    const storages::odbc::Query& query,
    OptionalCommandControl explicit_command_control
) const {
    UINVARIANT(command_control_store_, "ODBC transaction connection has no command-control store");
    return command_control_store_->ResolveTransactionStatement(
        std::move(transaction_base),
        query.GetOptionalNameView(),
        explicit_command_control
    );
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
