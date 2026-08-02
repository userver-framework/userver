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
#include <userver/utils/assert.hpp>
#include <userver/utils/datetime/steady_coarse_clock.hpp>

#include <storages/odbc/detail/broken_guard.hpp>
#include <storages/odbc/detail/bulk.hpp>
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

std::vector<detail::ResultWrapper::Column> DescribeResultColumns(SQLHSTMT statement, engine::Deadline deadline) {
    SQLSMALLINT column_count = 0;
    detail::CheckDeadlineNotExpired(deadline);
    const auto column_count_result = SQLNumResultCols(statement, &column_count);
    HandleStatementWarnings(column_count_result, statement, "determining result column count", false);
    detail::CheckDeadlineNotExpired(deadline);
    CheckStatementResult(column_count_result, statement, "get result column count for");

    std::vector<detail::ResultWrapper::Column> columns;
    columns.reserve(static_cast<std::size_t>(column_count));
    for (SQLSMALLINT index = 0; index < column_count; ++index) {
        columns.push_back(DescribeColumn(statement, static_cast<SQLUSMALLINT>(index + 1), deadline));
    }
    return columns;
}

std::pair<std::vector<detail::ResultWrapper::Row>, bool> FetchResultRows(
    SQLHSTMT statement,
    const std::vector<detail::ResultWrapper::Column>& columns,
    std::size_t max_rows,
    engine::Deadline deadline
) {
    std::vector<detail::ResultWrapper::Row> rows;
    constexpr std::size_t kMaxCursorReserve = 1024;
    rows.reserve(std::min(max_rows, kMaxCursorReserve));
    bool observed_eof = false;
    while (rows.size() < max_rows) {
        detail::CheckDeadlineNotExpired(deadline);
        const auto fetch_result = SQLFetch(statement);
        HandleStatementWarnings(fetch_result, statement, "fetching a result row", false);
        detail::CheckDeadlineNotExpired(deadline);
        if (fetch_result == SQL_NO_DATA) {
            observed_eof = true;
            break;
        }
        CheckStatementResult(fetch_result, statement, "fetch row from");

        detail::ResultWrapper::Row row;
        row.reserve(columns.size());
        for (std::size_t index = 0; index < columns.size(); ++index) {
            row.push_back(ReadCell(statement, static_cast<SQLUSMALLINT>(index + 1), columns[index], deadline));
        }
        rows.push_back(std::move(row));
    }
    return {std::move(rows), observed_eof};
}

std::shared_ptr<detail::ResultWrapper> MaterializeResult(SQLHSTMT statement, engine::Deadline deadline) {
    auto columns = DescribeResultColumns(statement, deadline);

    std::size_t rows_affected = 0;
    if (columns.empty()) {
        SQLLEN affected = 0;
        detail::CheckDeadlineNotExpired(deadline);
        const auto row_count_result = SQLRowCount(statement, &affected);
        HandleStatementWarnings(row_count_result, statement, "determining the affected row count", false);
        detail::CheckDeadlineNotExpired(deadline);
        rows_affected = SQL_SUCCEEDED(row_count_result) && affected > 0 ? static_cast<std::size_t>(affected) : 0;
    }

    std::vector<detail::ResultWrapper::Row> rows;
    if (!columns.empty()) {
        auto all_rows = FetchResultRows(statement, columns, std::numeric_limits<std::size_t>::max(), deadline);
        rows = std::move(all_rows.first);
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

struct CursorParameterBuffers final {
    std::vector<BoundParameter> parameters;
    std::vector<SQLLEN> indicators;
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

void CleanupPreparedStatement(SQLHSTMT statement) {
    std::exception_ptr first_error;
    const auto cleanup = [&](SQLUSMALLINT option, std::string_view operation) {
        try {
            const auto result = SQLFreeStmt(statement, option);
            HandleStatementWarnings(result, statement, operation, false);
            CheckStatementResult(result, statement, operation);
        } catch (...) {
            if (!first_error) {
                first_error = std::current_exception();
            }
        }
    };
    cleanup(SQL_CLOSE, "close a prepared query cursor and discard pending results");
    cleanup(SQL_RESET_PARAMS, "reset prepared query parameter bindings");
    if (first_error) {
        std::rethrow_exception(first_error);
    }
}

void SetCursorQueryTimeout(SQLHSTMT statement, engine::Deadline deadline) {
    SQLULEN timeout_seconds = 0;
    if (deadline.IsReachable()) {
        const auto left = deadline.TimeLeft();
        if (left <= engine::Deadline::Duration::zero()) {
            throw OperationInterrupted("Cancelled by deadline");
        }
        timeout_seconds = static_cast<SQLULEN>(std::chrono::ceil<std::chrono::seconds>(left).count());
    }
    const auto result = SQLSetStmtAttr(
        statement,
        SQL_ATTR_QUERY_TIMEOUT,
        reinterpret_cast<SQLPOINTER>(static_cast<std::uintptr_t>(timeout_seconds)),
        0
    );
    CheckStatementResult(result, statement, "set query timeout on");
    HandleStatementWarnings(result, statement, "setting a cursor query timeout", false);
}

void ResetCursorStatementAttributes(SQLHSTMT statement) {
    const auto result = SQLSetStmtAttr(statement, SQL_ATTR_QUERY_TIMEOUT, reinterpret_cast<SQLPOINTER>(0), 0);
    CheckStatementResult(result, statement, "reset query timeout on");
    HandleStatementWarnings(result, statement, "resetting a cursor query timeout", false);
}

void StartPreparedStatement(
    SQLHSTMT statement,
    const impl::ParameterList& parameters,
    engine::Deadline deadline,
    bool& execution_started,
    std::shared_ptr<void>& parameter_buffers
) {
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

    auto buffers = std::make_shared<CursorParameterBuffers>();
    // Publish ownership before the first bind. ODBC retains every data and
    // indicator pointer until SQL_RESET_PARAMS, including on bind/execute
    // failures.
    parameter_buffers = buffers;
    auto& bound_parameters = buffers->parameters;
    bound_parameters.reserve(parameters.size());
    for (const auto& parameter : parameters) {
        bound_parameters.emplace_back(parameter);
    }

    auto& indicators = buffers->indicators;
    indicators.resize(bound_parameters.size());
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
    execution_started = true;
    const auto execute_result = SQLExecute(statement);
    HandleStatementWarnings(execute_result, statement, "executing a prepared query", true);
    detail::CheckDeadlineNotExpired(deadline);
    if (!SQL_SUCCEEDED(execute_result) && execute_result != SQL_NO_DATA) {
        throw MakeDriverError<
            StatementError>("Failed to execute prepared ODBC query", execute_result, statement, SQL_HANDLE_STMT);
    }
}

ResultSet ExecutePreparedStatement(
    SQLHSTMT statement,
    const impl::ParameterList& parameters,
    engine::Deadline deadline,
    bool& execution_started,
    std::shared_ptr<void>& parameter_buffers
) {
    bool cleanup_attempted = false;
    try {
        StartPreparedStatement(statement, parameters, deadline, execution_started, parameter_buffers);
        auto result = ResultSet(MaterializeResult(statement, deadline));
        cleanup_attempted = true;
        CleanupPreparedStatement(statement);
        parameter_buffers.reset();
        return result;
    } catch (...) {
        const auto original_error = std::current_exception();
        if (!cleanup_attempted) {
            try {
                CleanupPreparedStatement(statement);
                parameter_buffers.reset();
            } catch (const std::exception& cleanup_error) {
                LOG_WARNING() << "Failed to clean up an erroneous ODBC prepared statement: " << cleanup_error;
            } catch (...) {
                LOG_WARNING() << "Failed to clean up an erroneous ODBC prepared statement";
            }
        }
        std::rethrow_exception(original_error);
    }
}

SQLRETURN ExecuteDirectStatement(SQLHSTMT statement, std::string_view query, engine::Deadline deadline) {
    std::vector<SQLCHAR> query_buffer(query.begin(), query.end());
    query_buffer.push_back('\0');

    detail::CheckDeadlineNotExpired(deadline);
    const auto execute_result = SQLExecDirect(statement, query_buffer.data(), SQL_NTS);
    HandleStatementWarnings(execute_result, statement, "executing a direct query", true);
    detail::CheckDeadlineNotExpired(deadline);
    return execute_result;
}

bool HasOnlyWarningClassDiagnostics(const std::vector<DiagnosticRecord>& diagnostics) {
    return std::all_of(diagnostics.begin(), diagnostics.end(), [](const DiagnosticRecord& diagnostic) {
        return diagnostic.sql_state.size() >= 2 && diagnostic.sql_state.compare(0, 2, "01") == 0;
    });
}

void CheckBulkStatementCall(SQLRETURN result, SQLHSTMT statement, std::string_view operation) {
    auto diagnostics =
        result == SQL_SUCCESS ? std::vector<DiagnosticRecord>{} : detail::GetSQLDiagnostics(statement, SQL_HANDLE_STMT);
    if (!SQL_SUCCEEDED(result)) {
        throw MakeDriverError<
            StatementError>(fmt::format("Failed to {}", operation), result, statement, SQL_HANDLE_STMT);
    }
    if (result == SQL_SUCCESS_WITH_INFO) {
        if (!HasOnlyWarningClassDiagnostics(diagnostics)) {
            throw MakeDiagnosticError<StatementError>(
                fmt::format("ODBC driver returned error-class diagnostics while {}", operation),
                diagnostics
            );
        }
        LogOdbcWarnings(operation, diagnostics);
    }
}

bool IsUnsupportedBulkAttribute(const std::vector<DiagnosticRecord>& diagnostics) {
    const auto is_explicit = [](const DiagnosticRecord& diagnostic) {
        return diagnostic.sql_state == "HYC00" || diagnostic.sql_state == "HY092" || diagnostic.sql_state == "IM001" ||
               diagnostic.sql_state == "01S02";
    };
    const auto is_allowed = [&](const DiagnosticRecord& diagnostic) {
        return is_explicit(diagnostic) ||
               (diagnostic.sql_state.size() >= 2 && diagnostic.sql_state.compare(0, 2, "01") == 0);
    };
    return std::any_of(diagnostics.begin(), diagnostics.end(), is_explicit) &&
           std::all_of(diagnostics.begin(), diagnostics.end(), is_allowed);
}

void ConfigureDecimalDescriptor(
    SQLHSTMT statement,
    SQLUSMALLINT parameter,
    SQLULEN precision,
    SQLSMALLINT scale,
    SQLPOINTER data,
    engine::Deadline deadline
) {
    SQLHDESC descriptor = SQL_NULL_HDESC;
    detail::CheckDeadlineNotExpired(deadline);
    const auto descriptor_result =
        SQLGetStmtAttr(statement, SQL_ATTR_APP_PARAM_DESC, &descriptor, sizeof(descriptor), nullptr);
    detail::CheckDeadlineNotExpired(deadline);
    if (!SQL_SUCCEEDED(descriptor_result)) {
        throw MakeDriverError<StatementError>(
            fmt::format("Failed to obtain the descriptor for ODBC Decimal parameter {}", parameter),
            descriptor_result,
            statement,
            SQL_HANDLE_STMT
        );
    }
    if (descriptor_result == SQL_SUCCESS_WITH_INFO) {
        LogOdbcWarnings(
            fmt::format("Obtaining the descriptor for ODBC Decimal parameter {}", parameter),
            statement,
            SQL_HANDLE_STMT
        );
    }

    const auto record = static_cast<SQLSMALLINT>(parameter);
    const auto numeric_field = [](SQLLEN value) {
        return reinterpret_cast<SQLPOINTER>(static_cast<std::uintptr_t>(value));
    };
    const auto set_field = [&](SQLSMALLINT field, SQLPOINTER value, std::string_view name) {
        detail::CheckDeadlineNotExpired(deadline);
        const auto result = SQLSetDescField(descriptor, record, field, value, 0);
        const auto warnings = GetWarnings(result, descriptor, SQL_HANDLE_DESC);
        detail::CheckDeadlineNotExpired(deadline);
        if (!SQL_SUCCEEDED(result)) {
            throw MakeDriverError<StatementError>(
                fmt::format("Failed to set {} for ODBC Decimal parameter {}", name, parameter),
                result,
                descriptor,
                SQL_HANDLE_DESC
            );
        }
        if (result == SQL_SUCCESS_WITH_INFO) {
            LogOdbcWarnings(fmt::format("Setting {} for ODBC Decimal parameter {}", name, parameter), warnings);
        }
    };

    // Changing non-deferred APD fields clears SQL_DESC_DATA_PTR.
    set_field(SQL_DESC_TYPE, numeric_field(SQL_C_NUMERIC), "C type");
    set_field(SQL_DESC_PRECISION, numeric_field(static_cast<SQLLEN>(precision)), "precision");
    set_field(SQL_DESC_SCALE, numeric_field(scale), "scale");
    set_field(SQL_DESC_DATA_PTR, data, "data pointer");
}

void ValidateBulkDmlStatement(
    SQLHSTMT statement,
    std::size_t expected_parameters,
    bool validate_result_columns,
    engine::Deadline deadline
) {
    SQLSMALLINT actual_parameters = 0;
    detail::CheckDeadlineNotExpired(deadline);
    const auto count_result = SQLNumParams(statement, &actual_parameters);
    detail::CheckDeadlineNotExpired(deadline);
    CheckBulkStatementCall(count_result, statement, "determine bulk parameter count");
    if (actual_parameters < 0 || static_cast<std::size_t>(actual_parameters) != expected_parameters) {
        throw StatementError(fmt::format(
            "ODBC parameter count mismatch: query expects {}, bulk rows contain {}",
            actual_parameters,
            expected_parameters
        ));
    }

    if (!validate_result_columns) {
        return;
    }

    SQLSMALLINT result_columns = 0;
    detail::CheckDeadlineNotExpired(deadline);
    const auto columns_result = SQLNumResultCols(statement, &result_columns);
    detail::CheckDeadlineNotExpired(deadline);
    CheckBulkStatementCall(columns_result, statement, "check bulk DML result columns");
    if (result_columns != 0) {
        throw StatementError("ODBC ExecuteBulk accepts DML without result sets only");
    }
}

struct BulkDmlResult final {
    std::optional<std::size_t> rows_affected{0};
};

BulkDmlResult DrainBulkDml(SQLHSTMT statement, engine::Deadline deadline) {
    BulkDmlResult result;
    while (true) {
        SQLSMALLINT result_columns = 0;
        detail::CheckDeadlineNotExpired(deadline);
        const auto columns_result = SQLNumResultCols(statement, &result_columns);
        detail::CheckDeadlineNotExpired(deadline);
        CheckBulkStatementCall(columns_result, statement, "check a bulk DML result");
        if (result_columns != 0) {
            throw StatementError("ODBC bulk DML produced a result set after execution");
        }

        SQLLEN row_count = -1;
        detail::CheckDeadlineNotExpired(deadline);
        const auto row_count_result = SQLRowCount(statement, &row_count);
        detail::CheckDeadlineNotExpired(deadline);
        CheckBulkStatementCall(row_count_result, statement, "read bulk DML row count");
        if (row_count < 0) {
            result.rows_affected.reset();
        } else if (result.rows_affected) {
            const auto count = static_cast<std::size_t>(row_count);
            if (count > std::numeric_limits<std::size_t>::max() - *result.rows_affected) {
                throw StatementError("ODBC bulk aggregate row count overflows size_t");
            }
            *result.rows_affected += count;
        }

        detail::CheckDeadlineNotExpired(deadline);
        const auto more_result = SQLMoreResults(statement);
        const auto more_diagnostics =
            more_result == SQL_SUCCESS
                ? std::vector<DiagnosticRecord>{}
                : detail::GetSQLDiagnostics(statement, SQL_HANDLE_STMT);
        detail::CheckDeadlineNotExpired(deadline);
        if (more_result == SQL_NO_DATA) {
            break;
        }
        if (!SQL_SUCCEEDED(more_result)) {
            throw MakeDiagnosticError<StatementError>("Failed to drain ODBC bulk DML results", more_diagnostics);
        }
        if (more_result == SQL_SUCCESS_WITH_INFO) {
            if (!HasOnlyWarningClassDiagnostics(more_diagnostics)) {
                throw MakeDiagnosticError<StatementError>(
                    "ODBC driver returned error-class diagnostics while draining bulk DML results",
                    more_diagnostics
                );
            }
            LogOdbcWarnings("Draining ODBC bulk DML results", more_diagnostics);
        }
    }
    return result;
}

void CleanupBulkStatement(SQLHSTMT statement) {
    std::exception_ptr first_error;
    const auto attempt = [&](auto&& operation) {
        try {
            operation();
        } catch (...) {
            if (!first_error) {
                first_error = std::current_exception();
            }
        }
    };
    const auto free_statement = [&](SQLUSMALLINT option, std::string_view operation) {
        const auto result = SQLFreeStmt(statement, option);
        HandleStatementWarnings(result, statement, operation, false);
        CheckStatementResult(result, statement, operation);
    };
    const auto reset_attribute = [&](SQLINTEGER attribute, SQLPOINTER value, std::string_view operation) {
        const auto result = SQLSetStmtAttr(statement, attribute, value, 0);
        auto diagnostics =
            result == SQL_SUCCESS
                ? std::vector<DiagnosticRecord>{}
                : detail::GetSQLDiagnostics(statement, SQL_HANDLE_STMT);
        if (!SQL_SUCCEEDED(result)) {
            throw MakeDiagnosticError<StatementError>(fmt::format("Failed to {}", operation), diagnostics);
        }
        if (result == SQL_SUCCESS_WITH_INFO) {
            if (std::any_of(
                    diagnostics.begin(),
                    diagnostics.end(),
                    [](const DiagnosticRecord& diagnostic) { return diagnostic.sql_state == "01S02"; }
                ) ||
                !HasOnlyWarningClassDiagnostics(diagnostics))
            {
                throw MakeDiagnosticError<
                    StatementError>(fmt::format("ODBC driver did not exactly {}", operation), diagnostics);
            }
            LogOdbcWarnings(operation, diagnostics);
        }
    };
    const auto verify_integer_attribute = [&](SQLINTEGER attribute, SQLULEN expected, std::string_view operation) {
        SQLULEN actual = 0;
        const auto result = SQLGetStmtAttr(statement, attribute, &actual, sizeof(actual), nullptr);
        auto diagnostics =
            result == SQL_SUCCESS
                ? std::vector<DiagnosticRecord>{}
                : detail::GetSQLDiagnostics(statement, SQL_HANDLE_STMT);
        if (!SQL_SUCCEEDED(result) || actual != expected) {
            throw MakeDiagnosticError<StatementError>(fmt::format("Failed to {}", operation), diagnostics);
        }
        if (result == SQL_SUCCESS_WITH_INFO) {
            if (!HasOnlyWarningClassDiagnostics(diagnostics)) {
                throw MakeDiagnosticError<StatementError>(fmt::format("Failed to {}", operation), diagnostics);
            }
            LogOdbcWarnings(operation, diagnostics);
        }
    };
    const auto verify_pointer_attribute = [&](SQLINTEGER attribute, std::string_view operation) {
        SQLPOINTER actual = reinterpret_cast<SQLPOINTER>(static_cast<std::uintptr_t>(1));
        const auto result = SQLGetStmtAttr(statement, attribute, &actual, sizeof(actual), nullptr);
        auto diagnostics =
            result == SQL_SUCCESS
                ? std::vector<DiagnosticRecord>{}
                : detail::GetSQLDiagnostics(statement, SQL_HANDLE_STMT);
        if ((!SQL_SUCCEEDED(result) && !IsUnsupportedBulkAttribute(diagnostics)) ||
            (SQL_SUCCEEDED(result) && actual != nullptr))
        {
            throw MakeDiagnosticError<StatementError>(fmt::format("Failed to {}", operation), diagnostics);
        }
        if (result == SQL_SUCCESS_WITH_INFO && !IsUnsupportedBulkAttribute(diagnostics)) {
            if (!HasOnlyWarningClassDiagnostics(diagnostics)) {
                throw MakeDiagnosticError<StatementError>(fmt::format("Failed to {}", operation), diagnostics);
            }
            LogOdbcWarnings(operation, diagnostics);
        }
    };

    attempt([&] { free_statement(SQL_CLOSE, "close a bulk DML cursor and discard pending results"); });
    attempt([&] { free_statement(SQL_RESET_PARAMS, "reset bulk DML parameter bindings"); });
    attempt([&] { reset_attribute(SQL_ATTR_PARAM_STATUS_PTR, nullptr, "reset bulk DML status pointer"); });
    attempt([&] { reset_attribute(SQL_ATTR_PARAMS_PROCESSED_PTR, nullptr, "reset bulk DML processed pointer"); });
    attempt([&] {
        reset_attribute(
            SQL_ATTR_PARAMSET_SIZE,
            reinterpret_cast<SQLPOINTER>(static_cast<std::uintptr_t>(1)),
            "reset bulk DML parameter-set size"
        );
    });
    attempt([&] { verify_integer_attribute(SQL_ATTR_PARAMSET_SIZE, 1, "verify bulk DML parameter-set reset"); });
    attempt([&] {
        verify_integer_attribute(SQL_ATTR_PARAM_BIND_TYPE, SQL_PARAM_BIND_BY_COLUMN, "verify bulk DML bind-type reset");
    });
    attempt([&] { verify_pointer_attribute(SQL_ATTR_PARAM_STATUS_PTR, "verify bulk DML status-pointer reset"); });
    attempt([&] { verify_pointer_attribute(SQL_ATTR_PARAMS_PROCESSED_PTR, "verify bulk DML processed-pointer reset"); }
    );
    attempt([&] {
        reset_attribute(
            SQL_ATTR_PARAM_BIND_TYPE,
            reinterpret_cast<SQLPOINTER>(static_cast<std::uintptr_t>(SQL_PARAM_BIND_BY_COLUMN)),
            "reset bulk DML bind type"
        );
    });
    if (first_error) {
        std::rethrow_exception(first_error);
    }
}

enum class BulkAttributeSetup { kNative, kFallback };

BulkAttributeSetup SetBulkAttribute(
    SQLHSTMT statement,
    SQLINTEGER attribute,
    SQLPOINTER value,
    std::string_view name,
    engine::Deadline deadline
) {
    detail::CheckDeadlineNotExpired(deadline);
    const auto result = SQLSetStmtAttr(statement, attribute, value, 0);
    auto diagnostics =
        result == SQL_SUCCESS ? std::vector<DiagnosticRecord>{} : detail::GetSQLDiagnostics(statement, SQL_HANDLE_STMT);
    detail::CheckDeadlineNotExpired(deadline);
    if ((!SQL_SUCCEEDED(result) || result == SQL_SUCCESS_WITH_INFO) && IsUnsupportedBulkAttribute(diagnostics)) {
        return BulkAttributeSetup::kFallback;
    }
    if (!SQL_SUCCEEDED(result)) {
        throw MakeDiagnosticError<
            StatementError>(fmt::format("Failed to set ODBC bulk attribute {}", name), diagnostics);
    }
    if (result == SQL_SUCCESS_WITH_INFO) {
        LogOdbcWarnings(fmt::format("Setting ODBC bulk attribute {}", name), diagnostics);
    }
    return BulkAttributeSetup::kNative;
}

BulkAttributeSetup ConfigureNativeBulkAttributes(
    SQLHSTMT statement,
    detail::BulkBindings& bindings,
    engine::Deadline deadline
) {
    const auto set = [&](SQLINTEGER attribute, SQLPOINTER value, std::string_view name) {
        return SetBulkAttribute(statement, attribute, value, name, deadline);
    };
    if (set(SQL_ATTR_PARAM_BIND_TYPE,
            reinterpret_cast<SQLPOINTER>(static_cast<std::uintptr_t>(SQL_PARAM_BIND_BY_COLUMN)),
            "SQL_ATTR_PARAM_BIND_TYPE") == BulkAttributeSetup::kFallback ||
        set(SQL_ATTR_PARAMSET_SIZE,
            reinterpret_cast<SQLPOINTER>(static_cast<std::uintptr_t>(bindings.rows_count)),
            "SQL_ATTR_PARAMSET_SIZE") == BulkAttributeSetup::kFallback ||
        set(SQL_ATTR_PARAM_STATUS_PTR, bindings.statuses.data(), "SQL_ATTR_PARAM_STATUS_PTR"
        ) == BulkAttributeSetup::kFallback ||
        set(SQL_ATTR_PARAMS_PROCESSED_PTR, &bindings.processed, "SQL_ATTR_PARAMS_PROCESSED_PTR"
        ) == BulkAttributeSetup::kFallback)
    {
        return BulkAttributeSetup::kFallback;
    }

    SQLULEN actual_size = 0;
    detail::CheckDeadlineNotExpired(deadline);
    const auto
        get_result = SQLGetStmtAttr(statement, SQL_ATTR_PARAMSET_SIZE, &actual_size, sizeof(actual_size), nullptr);
    auto diagnostics =
        get_result == SQL_SUCCESS
            ? std::vector<DiagnosticRecord>{}
            : detail::GetSQLDiagnostics(statement, SQL_HANDLE_STMT);
    detail::CheckDeadlineNotExpired(deadline);
    if ((!SQL_SUCCEEDED(get_result) || get_result == SQL_SUCCESS_WITH_INFO) && IsUnsupportedBulkAttribute(diagnostics))
    {
        return BulkAttributeSetup::kFallback;
    }
    if (!SQL_SUCCEEDED(get_result)) {
        throw MakeDiagnosticError<StatementError>("Failed to read back ODBC bulk parameter-set size", diagnostics);
    }
    if (get_result == SQL_SUCCESS_WITH_INFO) {
        LogOdbcWarnings("Reading back ODBC bulk parameter-set size", diagnostics);
    }
    if (actual_size != bindings.rows_count) {
        return BulkAttributeSetup::kFallback;
    }

    SQLULEN actual_bind_type = 0;
    detail::CheckDeadlineNotExpired(deadline);
    const auto bind_type_result =
        SQLGetStmtAttr(statement, SQL_ATTR_PARAM_BIND_TYPE, &actual_bind_type, sizeof(actual_bind_type), nullptr);
    diagnostics =
        bind_type_result == SQL_SUCCESS
            ? std::vector<DiagnosticRecord>{}
            : detail::GetSQLDiagnostics(statement, SQL_HANDLE_STMT);
    detail::CheckDeadlineNotExpired(deadline);
    if ((!SQL_SUCCEEDED(bind_type_result) || bind_type_result == SQL_SUCCESS_WITH_INFO) &&
        IsUnsupportedBulkAttribute(diagnostics))
    {
        return BulkAttributeSetup::kFallback;
    }
    if (!SQL_SUCCEEDED(bind_type_result)) {
        throw MakeDiagnosticError<StatementError>("Failed to read back ODBC bulk parameter bind type", diagnostics);
    }
    if (bind_type_result == SQL_SUCCESS_WITH_INFO) {
        LogOdbcWarnings("Reading back ODBC bulk parameter bind type", diagnostics);
    }
    return actual_bind_type == SQL_PARAM_BIND_BY_COLUMN ? BulkAttributeSetup::kNative : BulkAttributeSetup::kFallback;
}

void BindNativeBulkParameters(SQLHSTMT statement, detail::BulkBindings& bindings, engine::Deadline deadline) {
    for (std::size_t column = 0; column < bindings.columns.size(); ++column) {
        auto& binding = bindings.columns[column];
        const auto parameter = static_cast<SQLUSMALLINT>(column + 1);
        detail::CheckDeadlineNotExpired(deadline);
        const auto bind_result = SQLBindParameter(
            statement,
            parameter,
            SQL_PARAM_INPUT,
            binding.c_type,
            binding.sql_type,
            binding.column_size,
            binding.decimal_digits,
            binding.Data(),
            binding.sql_type == SQL_DECIMAL ? SQLLEN{0} : binding.buffer_size,
            binding.indicators.data()
        );
        HandleStatementWarnings(bind_result, statement, "binding a bulk DML parameter", false);
        detail::CheckDeadlineNotExpired(deadline);
        if (!SQL_SUCCEEDED(bind_result)) {
            throw MakeDriverError<StatementError>(
                fmt::format("Failed to bind ODBC bulk parameter {}", parameter),
                bind_result,
                statement,
                SQL_HANDLE_STMT
            );
        }
        if (binding.sql_type == SQL_DECIMAL) {
            ConfigureDecimalDescriptor(
                statement,
                parameter,
                binding.column_size,
                binding.decimal_digits,
                binding.Data(),
                deadline
            );
        }
    }
}

struct ScalarBulkExecution final {
    BulkRowStatus status{BulkRowStatus::kSuccess};
    std::optional<std::size_t> rows_affected{0};
};

void SetBulkStatementTimeout(SQLHSTMT statement, engine::Deadline deadline) {
    SQLULEN timeout_seconds = 0;
    if (deadline.IsReachable()) {
        const auto left = deadline.TimeLeft();
        if (left <= engine::Deadline::Duration::zero()) {
            throw OperationInterrupted("Cancelled by deadline");
        }
        timeout_seconds = static_cast<SQLULEN>(std::chrono::ceil<std::chrono::seconds>(left).count());
    }
    const auto result = SQLSetStmtAttr(
        statement,
        SQL_ATTR_QUERY_TIMEOUT,
        reinterpret_cast<SQLPOINTER>(static_cast<std::uintptr_t>(timeout_seconds)),
        0
    );
    if (!SQL_SUCCEEDED(result)) {
        throw MakeDriverError<
            StatementError>("Failed to set ODBC bulk query timeout", result, statement, SQL_HANDLE_STMT);
    }
}

ScalarBulkExecution ExecuteScalarBulkDml(
    SQLHSTMT statement,
    const impl::ParameterList& parameters,
    engine::Deadline deadline,
    bool& execution_started,
    const std::function<void()>& discard_statement
) {
    std::vector<BoundParameter> bound_parameters;
    bound_parameters.reserve(parameters.size());
    for (const auto& parameter : parameters) {
        bound_parameters.emplace_back(parameter);
    }
    std::vector<SQLLEN> indicators(bound_parameters.size());
    try {
        for (std::size_t index = 0; index < bound_parameters.size(); ++index) {
            auto& parameter = bound_parameters[index];
            auto binding = GetParameterBinding(parameter);
            indicators[index] = parameter.is_null ? SQL_NULL_DATA : binding.buffer_size;
            const auto
                bind_buffer_size = parameter.type == impl::ParameterType::kDecimal ? SQLLEN{0} : binding.buffer_size;
            const auto parameter_number = static_cast<SQLUSMALLINT>(index + 1);
            detail::CheckDeadlineNotExpired(deadline);
            const auto bind_result = SQLBindParameter(
                statement,
                parameter_number,
                SQL_PARAM_INPUT,
                binding.c_type,
                binding.sql_type,
                binding.column_size,
                binding.decimal_digits,
                binding.data,
                bind_buffer_size,
                &indicators[index]
            );
            HandleStatementWarnings(bind_result, statement, "binding a scalar bulk fallback parameter", false);
            detail::CheckDeadlineNotExpired(deadline);
            if (!SQL_SUCCEEDED(bind_result)) {
                throw MakeDriverError<StatementError>(
                    fmt::format("Failed to bind ODBC scalar bulk fallback parameter {}", parameter_number),
                    bind_result,
                    statement,
                    SQL_HANDLE_STMT
                );
            }
            if (parameter.type == impl::ParameterType::kDecimal) {
                ConfigureDecimalDescriptor(
                    statement,
                    parameter_number,
                    binding.column_size,
                    binding.decimal_digits,
                    binding.data,
                    deadline
                );
            }
        }

        SetBulkStatementTimeout(statement, deadline);
        detail::CheckDeadlineNotExpired(deadline);
        execution_started = true;
        const auto execute_result = SQLExecute(statement);
        auto diagnostics =
            execute_result == SQL_SUCCESS
                ? std::vector<DiagnosticRecord>{}
                : detail::GetSQLDiagnostics(statement, SQL_HANDLE_STMT);
        detail::CheckDeadlineNotExpired(deadline);
        if (!SQL_SUCCEEDED(execute_result) && execute_result != SQL_NO_DATA) {
            throw MakeDriverError<StatementError>(
                "Failed to execute ODBC scalar bulk fallback",
                execute_result,
                statement,
                SQL_HANDLE_STMT
            );
        }
        if (execute_result == SQL_SUCCESS_WITH_INFO && !HasOnlyWarningClassDiagnostics(diagnostics)) {
            throw MakeDiagnosticError<
                StatementError>("ODBC scalar bulk fallback returned error-class diagnostics", diagnostics);
        }
        if (execute_result == SQL_SUCCESS_WITH_INFO) {
            LogOdbcWarnings("Executing ODBC scalar bulk fallback", diagnostics);
        }
        auto drained = DrainBulkDml(statement, deadline);
        CleanupPreparedStatement(statement);
        return ScalarBulkExecution{
            .status =
                execute_result == SQL_SUCCESS_WITH_INFO ? BulkRowStatus::kSuccessWithInfo : BulkRowStatus::kSuccess,
            .rows_affected = drained.rows_affected,
        };
    } catch (...) {
        const auto original_error = std::current_exception();
        bool cleanup_succeeded = false;
        try {
            CleanupPreparedStatement(statement);
            cleanup_succeeded = true;
        } catch (const std::exception& cleanup_error) {
            LOG_WARNING() << "Failed to clean up an erroneous scalar ODBC bulk statement: " << cleanup_error;
        } catch (...) {
            LOG_WARNING() << "Failed to clean up an erroneous scalar ODBC bulk statement";
        }
        if (!cleanup_succeeded) {
            discard_statement();
        }
        std::rethrow_exception(original_error);
    }
}

std::string FormatBulkErrorMessage(std::string message, const std::vector<DiagnosticRecord>& diagnostics) {
    const auto formatted = detail::FormatSQLDiagnostics(diagnostics);
    if (!formatted.empty()) {
        message += ": ";
        message += formatted;
    }
    return message;
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
    engine::Deadline deadline,
    std::shared_ptr<detail::PreparedStatementCacheState> prepared_statement_cache_state
)
    : blocking_task_processor_{blocking_task_processor},
      env_{SQL_NULL_HENV, &DestroyEnvironmentHandle},
      handle_{SQL_NULL_HDBC, &DestroyDatabaseHandle},
      prepared_statement_cache_state_{
          prepared_statement_cache_state
              ? std::move(prepared_statement_cache_state)
              : std::make_shared<detail::PreparedStatementCacheState>()
      },
      cursor_control_{std::make_shared<detail::CursorControl>(*this)}
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

Connection::~Connection() {
    InvalidateActiveCursor();
    {
        const std::lock_guard lock{cursor_control_->mutex_};
        cursor_control_->connection_ = nullptr;
    }
    try {
        RunBlocking(blocking_task_processor_, [this] {
            ClearPreparedStatements(false);
            DestroyDatabaseHandle(handle_.release());
            DestroyEnvironmentHandle(env_.release());
        });
    } catch (const std::exception& ex) {
        const auto size = prepared_statements_.GetSize();
        prepared_statements_.VisitAll([](const std::string&, CachedStatement& statement) {
            [[maybe_unused]] const auto leaked_statement = statement.handle.release();
        });
        prepared_statement_cache_state_->GetStatistics().current -= size;
        prepared_statements_.Clear();
        [[maybe_unused]] const auto leaked_database = handle_.release();
        [[maybe_unused]] const auto leaked_environment = env_.release();
        LOG_ERROR() << "Failed to schedule ODBC connection cleanup; leaking its handles: " << ex;
    } catch (...) {
        const auto size = prepared_statements_.GetSize();
        prepared_statements_.VisitAll([](const std::string&, CachedStatement& statement) {
            [[maybe_unused]] const auto leaked_statement = statement.handle.release();
        });
        prepared_statement_cache_state_->GetStatistics().current -= size;
        prepared_statements_.Clear();
        [[maybe_unused]] const auto leaked_database = handle_.release();
        [[maybe_unused]] const auto leaked_environment = env_.release();
        LOG_ERROR() << "Failed to schedule ODBC connection cleanup; leaking its handles";
    }
}

bool Connection::HasActiveCursor() const noexcept {
    const std::lock_guard lock{cursor_control_->mutex_};
    const auto active = cursor_control_->active_.lock();
    return active && !active->terminal.load();
}

void Connection::InvalidateActiveCursor() noexcept {
    detail::CursorLease lease;
    {
        const std::lock_guard lock{cursor_control_->mutex_};
        lease.control = cursor_control_;
        lease.statement = cursor_control_->active_.lock();
    }
    if (lease.statement) {
        CloseCursor(lease);
    }
}

std::chrono::microseconds Connection::TakeCursorTransactionBusyTime() noexcept {
    const std::lock_guard lock{cursor_control_->mutex_};
    return std::exchange(cursor_control_->transaction_busy_time_, std::chrono::microseconds{0});
}

detail::CursorLease Connection::OpenCursor(
    const storages::odbc::Query& query,
    const impl::ParameterList& parameters,
    engine::Deadline deadline,
    bool in_transaction,
    std::function<void(detail::CursorTerminalResult, std::chrono::microseconds)> on_terminal
) {
    auto statement = std::make_shared<detail::CursorStatement>();
    statement->in_transaction = in_transaction;
    statement->on_terminal = std::move(on_terminal);
    detail::CursorLease lease{cursor_control_, statement};
    {
        const std::lock_guard control_lock{cursor_control_->mutex_};
        const auto active = cursor_control_->active_.lock();
        if (active && !active->terminal.load()) {
            throw LogicError("Only one active ODBC cursor is allowed per connection");
        }
        cursor_control_->active_ = statement;
    }

    const auto query_text = query.GetStatementView();
    tracing::Span span{"odbc_cursor_open"};
    span.AddTag(tracing::kDatabaseType, "odbc");
    const auto span_tags = detail::tracing::MakeQuerySpanTags(query);
    if (span_tags.statement_name) {
        span.AddTag(tracing::kDatabaseStatementName, std::string{*span_tags.statement_name});
    }
    if (span_tags.statement) {
        span.AddTag(tracing::kDatabaseStatement, std::string{*span_tags.statement});
    }

    const auto started = utils::datetime::SteadyCoarseClock::now();
    try {
        CheckOperationInterrupted(deadline);
        RunBlockingChecked(blocking_task_processor_, deadline, [&, query_string = std::string{query_text}] {
            const std::lock_guard handle_lock{handle_mutex_};
            ApplyPreparedStatementCacheSettings();
            const auto cache_reset_generation = applied_prepared_cache_reset_generation_;

            // Cursor statements are always prepared so SQLNumResultCols can
            // reject DML before SQLExecute has any side effects. Zero-argument
            // statements are deliberately not inserted into the cache.
            const bool cacheable = !parameters.empty();
            const bool cache_enabled = cacheable && applied_prepared_cache_size_ != 0;
            CachedStatementMetadata cached_metadata;
            auto handle =
                cache_enabled
                    ? TakePreparedStatement(query_string, &cached_metadata)
                    : StatementHandle{nullptr, StatementHandleDeleter{this}};
            const bool was_cached = handle != nullptr;
            if (!handle) {
                handle = MakeStatementHandle();
            }

            try {
                SetCursorQueryTimeout(handle.get(), deadline);
                if (was_cached && cached_metadata.row_producing == false) {
                    throw StatementError("ODBC cursor requires a row-producing statement");
                }
                if (!was_cached || !cached_metadata.row_producing.has_value()) {
                    std::vector<SQLCHAR> query_buffer(query_string.begin(), query_string.end());
                    query_buffer.push_back('\0');
                    detail::CheckDeadlineNotExpired(deadline);
                    const auto prepare_result = SQLPrepare(handle.get(), query_buffer.data(), SQL_NTS);
                    HandleStatementWarnings(prepare_result, handle.get(), "preparing a cursor query", false);
                    detail::CheckDeadlineNotExpired(deadline);
                    CheckStatementResult(prepare_result, handle.get(), "prepare cursor query on");
                }

                std::vector<detail::ResultWrapper::Column> columns;
                if (!was_cached || !cached_metadata.row_producing.has_value()) {
                    columns = DescribeResultColumns(handle.get(), deadline);
                    if (columns.empty()) {
                        throw StatementError("ODBC cursor requires a row-producing statement");
                    }
                }
                StartPreparedStatement(
                    handle.get(),
                    parameters,
                    deadline,
                    statement->execution_started,
                    statement->parameter_buffers
                );
                if (was_cached && cached_metadata.row_producing == true) {
                    columns = DescribeResultColumns(handle.get(), deadline);
                    if (columns.empty()) {
                        throw StatementError("ODBC cursor requires a row-producing statement");
                    }
                }

                statement->columns = std::move(columns);
                statement->cache_key = query_string;
                statement->cache_reset_generation = cache_reset_generation;
                statement->prepared = true;
                statement->cacheable = cacheable;
                statement->was_cached = was_cached;
                statement->bulk_dml_validated = cached_metadata.bulk_dml_validated;
                statement->cache_allowed = IsInsideTransaction() || !InvalidatesPreparedStatements(SQL_COMMIT);
                statement->handle = handle.release();
            } catch (...) {
                const auto original_error = std::current_exception();
                bool cleanup_succeeded = false;
                try {
                    CleanupPreparedStatement(handle.get());
                    ResetCursorStatementAttributes(handle.get());
                    cleanup_succeeded = true;
                } catch (...) {
                    NotifyBroken();
                }
                AccountPreparedStatementFailure(was_cached);
                if (!cleanup_succeeded && statement->execution_started) {
                    NotifyBroken();
                }
                std::rethrow_exception(original_error);
            }
        });
        statement->busy_time += std::chrono::duration_cast<
            std::chrono::microseconds>(utils::datetime::SteadyCoarseClock::now() - started);
        return lease;
    } catch (const OperationInterrupted& ex) {
        statement->busy_time += std::chrono::duration_cast<
            std::chrono::microseconds>(utils::datetime::SteadyCoarseClock::now() - started);
        NotifyBroken();
        CloseCursor(lease, detail::CursorTerminalResult::kTimeout);
        span.AddTag(tracing::kErrorFlag, true);
        span.AddTag(tracing::kErrorMessage, ex.what());
        throw;
    } catch (const StatementError& ex) {
        statement->busy_time += std::chrono::duration_cast<
            std::chrono::microseconds>(utils::datetime::SteadyCoarseClock::now() - started);
        if (ex.IsInvalidHandle() || detail::HasConnectionError(ex.GetDiagnostics())) {
            NotifyBroken();
        } else {
            UpdateBrokenFromDriver();
        }
        CloseCursor(lease, detail::CursorTerminalResult::kError);
        span.AddTag(tracing::kErrorFlag, true);
        span.AddTag(tracing::kErrorMessage, ex.what());
        throw;
    } catch (const Error& ex) {
        statement->busy_time += std::chrono::duration_cast<
            std::chrono::microseconds>(utils::datetime::SteadyCoarseClock::now() - started);
        CloseCursor(lease, detail::CursorTerminalResult::kError);
        span.AddTag(tracing::kErrorFlag, true);
        span.AddTag(tracing::kErrorMessage, ex.what());
        throw;
    } catch (...) {
        statement->busy_time += std::chrono::duration_cast<
            std::chrono::microseconds>(utils::datetime::SteadyCoarseClock::now() - started);
        NotifyBroken();
        CloseCursor(lease, detail::CursorTerminalResult::kError);
        throw;
    }
}

ResultSet Connection::FetchCursor(detail::CursorLease& lease, std::size_t rows, engine::Deadline deadline) {
    auto& control = *lease.control;
    auto& statement = *lease.statement;
    Connection* connection = nullptr;
    std::vector<detail::ResultWrapper::Row> fetched_rows;
    bool observed_eof = false;
    bool fetch_started = false;
    bool elapsed_accounted = false;
    const auto started = utils::datetime::SteadyCoarseClock::now();
    const auto account_elapsed = [&] {
        if (!elapsed_accounted) {
            statement.busy_time += std::chrono::duration_cast<
                std::chrono::microseconds>(utils::datetime::SteadyCoarseClock::now() - started);
            elapsed_accounted = true;
        }
    };
    try {
        {
            const std::lock_guard control_lock{control.mutex_};
            if (statement.terminal.load() || statement.finalizing || control.active_.lock() != lease.statement) {
                throw LogicError("ODBC cursor is terminal");
            }
            if (statement.fetching) {
                throw LogicError("Concurrent Fetch calls on an ODBC cursor are not allowed");
            }
            connection = control.connection_;
            if (!connection) {
                throw LogicError("ODBC cursor connection is no longer available");
            }
            statement.fetching = true;
            fetch_started = true;
        }

        auto fetch_result = RunBlockingChecked(connection->blocking_task_processor_, deadline, [&] {
            const std::lock_guard handle_lock{connection->handle_mutex_};
            SetCursorQueryTimeout(statement.handle, deadline);
            return FetchResultRows(statement.handle, statement.columns, rows, deadline);
        });
        {
            const std::lock_guard control_lock{control.mutex_};
            fetched_rows = std::move(fetch_result.first);
            observed_eof = fetch_result.second;
            statement.fetching = false;
            account_elapsed();
            statement.fetched_so_far.fetch_add(fetched_rows.size());
        }
        control.finalized_cv_.NotifyAll();

        auto result = ResultSet(std::make_shared<detail::ResultWrapper>(statement.columns, std::move(fetched_rows), 0));
        if (observed_eof) {
            CloseCursor(lease);
        }
        return result;
    } catch (const OperationInterrupted&) {
        {
            const std::lock_guard control_lock{control.mutex_};
            account_elapsed();
            statement.fetching = false;
        }
        control.finalized_cv_.NotifyAll();
        if (connection) {
            connection->NotifyBroken();
        }
        CloseCursor(lease, detail::CursorTerminalResult::kTimeout);
        throw;
    } catch (const StatementError& ex) {
        {
            const std::lock_guard control_lock{control.mutex_};
            account_elapsed();
            statement.fetching = false;
        }
        control.finalized_cv_.NotifyAll();
        if (connection) {
            if (ex.IsInvalidHandle() || detail::HasConnectionError(ex.GetDiagnostics())) {
                connection->NotifyBroken();
            } else {
                connection->UpdateBrokenFromDriver();
            }
        }
        CloseCursor(lease, detail::CursorTerminalResult::kError);
        throw;
    } catch (const LogicError&) {
        if (fetch_started) {
            {
                const std::lock_guard control_lock{control.mutex_};
                account_elapsed();
                statement.fetching = false;
            }
            control.finalized_cv_.NotifyAll();
            CloseCursor(lease, detail::CursorTerminalResult::kError);
        }
        throw;
    } catch (...) {
        {
            const std::lock_guard control_lock{control.mutex_};
            account_elapsed();
            statement.fetching = false;
        }
        control.finalized_cv_.NotifyAll();
        CloseCursor(lease, detail::CursorTerminalResult::kError);
        throw;
    }
}

void Connection::CloseCursor(detail::CursorLease& lease, detail::CursorTerminalResult terminal_result) noexcept {
    if (!lease.control || !lease.statement) {
        return;
    }

    auto& control = *lease.control;
    auto& statement = *lease.statement;
    const engine::TaskCancellationBlocker cancellation_blocker;
    Connection* connection = nullptr;
    SQLHSTMT raw_handle = SQL_NULL_HSTMT;
    {
        std::unique_lock control_lock{control.mutex_};
        [[maybe_unused]] const auto finalized = control.finalized_cv_.Wait(control_lock, [&] {
            return !statement.finalizing || statement.terminal.load();
        });
        if (statement.terminal.load()) {
            return;
        }
        statement.finalizing = true;
        [[maybe_unused]] const auto fetch_finished = control.finalized_cv_.Wait(control_lock, [&] {
            return !statement.fetching;
        });
        connection = control.connection_;
        raw_handle = statement.handle;
        statement.handle = SQL_NULL_HSTMT;
    }

    const bool invalidates_cache =
        connection && !statement.in_transaction && statement.execution_started &&
        (terminal_result == detail::CursorTerminalResult::kSuccess
             ? connection->InvalidatesPreparedStatements(SQL_COMMIT)
             : (connection->InvalidatesPreparedStatements(SQL_COMMIT) ||
                connection->InvalidatesPreparedStatements(SQL_ROLLBACK)));
    bool cleanup_succeeded = raw_handle == SQL_NULL_HSTMT;
    bool cached_handle_accounted = false;
    const auto cleanup_started = utils::datetime::SteadyCoarseClock::now();
    if (connection && (raw_handle != SQL_NULL_HSTMT || invalidates_cache)) {
        try {
            const auto cleanup_deadline = engine::Deadline::FromDuration(detail::kDefaultCleanupTimeout);
            RunBlockingChecked(
                connection->blocking_task_processor_,
                cleanup_deadline,
                [connection,
                 &statement,
                 raw_handle,
                 terminal_result,
                 cleanup_deadline,
                 invalidates_cache,
                 &cached_handle_accounted] {
                    const std::lock_guard handle_lock{connection->handle_mutex_};
                    StatementHandle handle{raw_handle, StatementHandleDeleter{connection}};
                    if (handle) {
                        SetCursorQueryTimeout(handle.get(), cleanup_deadline);
                        CleanupPreparedStatement(handle.get());
                        ResetCursorStatementAttributes(handle.get());
                    }
                    connection->ApplyPreparedStatementCacheSettings();
                    if (invalidates_cache) {
                        if (handle && statement.was_cached) {
                            connection->AccountPreparedStatementFailure(statement.was_cached);
                            cached_handle_accounted = true;
                        }
                        connection->ClearPreparedStatements(true);
                    } else if (handle && statement.cacheable &&
                               terminal_result == detail::CursorTerminalResult::kSuccess && statement.cache_allowed)
                    {
                        connection->StorePreparedStatement(
                            statement.cache_key,
                            std::move(handle),
                            statement.was_cached,
                            statement.cache_reset_generation,
                            CachedStatementMetadata{
                                .row_producing = true,
                                .bulk_dml_validated = false,
                            }
                        );
                        cached_handle_accounted = true;
                    } else if (handle && statement.was_cached) {
                        connection->AccountPreparedStatementFailure(true);
                        cached_handle_accounted = true;
                    }
                    if (handle && !statement.prepared) {
                        const auto close_result = SQLFreeStmt(handle.get(), SQL_CLOSE);
                        CheckStatementResult(close_result, handle.get(), "close cursor on");
                        ResetCursorStatementAttributes(handle.get());
                        if (invalidates_cache) {
                            connection->ClearPreparedStatements(true);
                        }
                    }
                }
            );
            cleanup_succeeded = true;
        } catch (const std::exception& ex) {
            if (statement.was_cached && !cached_handle_accounted) {
                connection->AccountPreparedStatementFailure(true);
            }
            connection->NotifyBroken();
            LOG_ERROR() << "Failed to clean up an ODBC cursor statement: " << ex.what();
        } catch (...) {
            if (statement.was_cached && !cached_handle_accounted) {
                connection->AccountPreparedStatementFailure(true);
            }
            connection->NotifyBroken();
            LOG_ERROR() << "Failed to clean up an ODBC cursor statement";
        }
    } else if (raw_handle != SQL_NULL_HSTMT) {
        LOG_ERROR() << "ODBC cursor lost its connection before statement cleanup";
    }
    statement.busy_time += std::chrono::duration_cast<
        std::chrono::microseconds>(utils::datetime::SteadyCoarseClock::now() - cleanup_started);
    if (!cleanup_succeeded) {
        terminal_result = detail::CursorTerminalResult::kError;
    }

    std::function<void(detail::CursorTerminalResult, std::chrono::microseconds)> on_terminal;
    std::chrono::microseconds busy_time{0};
    {
        const std::lock_guard control_lock{control.mutex_};
        statement.parameter_buffers.reset();
        statement.closed = true;
        statement.finalizing = false;
        statement.terminal.store(true);
        if (control.active_.lock() == lease.statement) {
            control.active_.reset();
        }
        if (statement.in_transaction) {
            control.transaction_busy_time_ += statement.busy_time;
        }
        busy_time = statement.busy_time;
        on_terminal = std::move(statement.on_terminal);
    }
    control.finalized_cv_.NotifyAll();
    if (on_terminal) {
        try {
            on_terminal(terminal_result, busy_time);
        } catch (const std::exception& ex) {
            LOG_ERROR() << "ODBC cursor terminal callback failed: " << ex.what();
        } catch (...) {
            LOG_ERROR() << "ODBC cursor terminal callback failed";
        }
    }
}

Connection::CachedStatement::CachedStatement(std::string key, StatementHandle handle, CachedStatementMetadata metadata)
    : key{std::move(key)},
      handle{std::move(handle)},
      metadata{metadata}
{}

void Connection::StatementHandleDeleter::operator()(std::remove_pointer_t<SQLHSTMT>* handle) const noexcept {
    if (handle && connection) {
        connection->DestroyStatementHandle(handle);
    }
}

Connection::StatementHandle Connection::MakeStatementHandle() {
    SQLHSTMT statement = SQL_NULL_HSTMT;
    const auto result = SQLAllocHandle(SQL_HANDLE_STMT, handle_.get(), &statement);
    if (!SQL_SUCCEEDED(result)) {
        throw MakeDriverError<
            StatementError>("Failed to allocate ODBC statement handle", result, handle_.get(), SQL_HANDLE_DBC);
    }
    return StatementHandle{statement, StatementHandleDeleter{this}};
}

void Connection::DestroyStatementHandle(SQLHSTMT statement) noexcept {
    if (statement == SQL_NULL_HSTMT) {
        return;
    }
    const auto result = SQLFreeHandle(SQL_HANDLE_STMT, statement);
    if (!SQL_SUCCEEDED(result)) {
        NotifyBroken();
        try {
            LOG_ERROR()
                << "Failed to free an ODBC prepared statement handle: "
                << detail::FormatSQLDiagnostics(detail::GetSQLDiagnostics(statement, SQL_HANDLE_STMT));
        } catch (...) {
            // Handle cleanup is noexcept and the connection is already broken.
        }
    }
}

Connection::StatementHandle Connection::TakePreparedStatement(
    std::string_view query,
    CachedStatementMetadata* metadata
) {
    auto& statistics = prepared_statement_cache_state_->GetStatistics();
    auto* cached = prepared_statements_.GetTransparent(query);
    if (!cached) {
        ++statistics.misses;
        return StatementHandle{nullptr, StatementHandleDeleter{this}};
    }

    ++statistics.hits;
    if (metadata) {
        *metadata = cached->metadata;
    }
    auto handle = std::move(cached->handle);
    prepared_statements_.Erase(cached->key);
    return handle;
}

void Connection::StorePreparedStatement(
    std::string query,
    StatementHandle handle,
    bool was_cached,
    std::size_t operation_reset_generation,
    CachedStatementMetadata metadata
) {
    UASSERT_MSG(
        !metadata.bulk_dml_validated || metadata.row_producing == false,
        "A bulk-DML-validated ODBC cache entry must be known to produce no rows"
    );
    const auto current_settings = prepared_statement_cache_state_->GetSettings();
    if (applied_prepared_cache_size_ == 0 || current_settings.reset_generation != operation_reset_generation) {
        if (was_cached) {
            --prepared_statement_cache_state_->GetStatistics().current;
        }
        return;
    }

    if (prepared_statements_.GetSize() >= applied_prepared_cache_size_) {
        EvictLeastRecentlyUsedPreparedStatement();
    }
    prepared_statements_.Put(query, CachedStatement{query, std::move(handle), metadata});
    if (!was_cached) {
        ++prepared_statement_cache_state_->GetStatistics().current;
    }
}

void Connection::ApplyPreparedStatementCacheSettings() {
    const auto settings = prepared_statement_cache_state_->GetSettings();
    if (settings.reset_generation != applied_prepared_cache_reset_generation_) {
        ClearPreparedStatements(false);
        applied_prepared_cache_reset_generation_ = settings.reset_generation;
    }
    if (settings.max_size == applied_prepared_cache_size_) {
        return;
    }

    if (settings.max_size == 0) {
        ClearPreparedStatements(false);
    } else {
        while (prepared_statements_.GetSize() > settings.max_size) {
            EvictLeastRecentlyUsedPreparedStatement();
        }
        prepared_statements_.SetMaxSize(settings.max_size);
    }
    applied_prepared_cache_size_ = settings.max_size;
}

void Connection::ClearPreparedStatements(bool account_evictions) noexcept {
    const auto size = prepared_statements_.GetSize();
    if (size == 0) {
        return;
    }
    auto& statistics = prepared_statement_cache_state_->GetStatistics();
    statistics.current -= size;
    if (account_evictions) {
        statistics.evictions += utils::statistics::Rate{size};
    }
    prepared_statements_.Clear();
}

void Connection::EvictLeastRecentlyUsedPreparedStatement() noexcept {
    auto* statement = prepared_statements_.GetLeastUsed();
    if (!statement) {
        return;
    }
    prepared_statements_.Erase(statement->key);
    auto& statistics = prepared_statement_cache_state_->GetStatistics();
    --statistics.current;
    ++statistics.evictions;
}

void Connection::AccountPreparedStatementFailure(bool was_cached) noexcept {
    if (!was_cached) {
        return;
    }
    auto& statistics = prepared_statement_cache_state_->GetStatistics();
    --statistics.current;
    ++statistics.evictions;
}

bool Connection::InvalidatesPreparedStatements(SQLSMALLINT completion_type) const noexcept {
    const auto behavior =
        completion_type == SQL_COMMIT
            ? driver_capabilities_.GetCursorCommitBehavior()
            : driver_capabilities_.GetCursorRollbackBehavior();
    return !behavior || *behavior == detail::CursorBehavior::kDelete;
}

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
                        ApplyPreparedStatementCacheSettings();
                        const auto operation_cache_reset_generation = applied_prepared_cache_reset_generation_;

                        const auto set_query_timeout = [deadline](SQLHSTMT statement) {
                            SQLULEN timeout_sec = 0;
                            if (deadline.IsReachable()) {
                                const auto left = deadline.TimeLeft();
                                if (left <= engine::Deadline::Duration::zero()) {
                                    throw OperationInterrupted("Cancelled by deadline");
                                }
                                timeout_sec = static_cast<SQLULEN>(std::chrono::ceil<std::chrono::seconds>(left).count()
                                );
                            }
                            const auto timeout_result = SQLSetStmtAttr(
                                statement,
                                SQL_ATTR_QUERY_TIMEOUT,
                                reinterpret_cast<SQLPOINTER>(static_cast<std::uintptr_t>(timeout_sec)),
                                0
                            );
                            if (!SQL_SUCCEEDED(timeout_result)) {
                                throw MakeDriverError<StatementError>(
                                    "Failed to set ODBC query timeout",
                                    timeout_result,
                                    statement,
                                    SQL_HANDLE_STMT
                                );
                            }
                        };

                        if (parameters.empty()) {
                            auto statement_handle = MakeStatementHandle();
                            bool execution_started = false;
                            try {
                                set_query_timeout(statement_handle.get());
                                execution_started = true;
                                const auto result = ExecuteDirectStatement(statement_handle.get(), query, deadline);
                                if (!SQL_SUCCEEDED(result) && result != SQL_NO_DATA) {
                                    throw MakeDriverError<StatementError>(
                                        "Failed to execute query",
                                        result,
                                        statement_handle.get(),
                                        SQL_HANDLE_STMT
                                    );
                                }
                                auto materialized = ResultSet(MaterializeResult(statement_handle.get(), deadline));
                                if (!IsInsideTransaction() && InvalidatesPreparedStatements(SQL_COMMIT)) {
                                    ClearPreparedStatements(true);
                                }
                                return materialized;
                            } catch (...) {
                                if (execution_started && !IsInsideTransaction() &&
                                    (InvalidatesPreparedStatements(SQL_COMMIT) ||
                                     InvalidatesPreparedStatements(SQL_ROLLBACK)))
                                {
                                    ClearPreparedStatements(true);
                                }
                                throw;
                            }
                        }

                        const bool cache_enabled = applied_prepared_cache_size_ != 0;
                        CachedStatementMetadata cached_metadata;
                        // Keep this owner alive longer than statement_handle on
                        // cleanup failure, so bound pointers cannot dangle
                        // before the HSTMT is destroyed.
                        std::shared_ptr<void> parameter_buffers;
                        auto statement_handle =
                            cache_enabled
                                ? TakePreparedStatement(query, &cached_metadata)
                                : StatementHandle{nullptr, StatementHandleDeleter{this}};
                        const bool was_cached = statement_handle != nullptr;
                        if (!statement_handle) {
                            statement_handle = MakeStatementHandle();
                        }

                        bool execution_started = false;
                        try {
                            set_query_timeout(statement_handle.get());
                            if (!was_cached) {
                                std::vector<SQLCHAR> query_buffer(query.begin(), query.end());
                                query_buffer.push_back('\0');
                                detail::CheckDeadlineNotExpired(deadline);
                                const auto
                                    prepare_result = SQLPrepare(statement_handle.get(), query_buffer.data(), SQL_NTS);
                                HandleStatementWarnings(
                                    prepare_result,
                                    statement_handle.get(),
                                    "preparing a query",
                                    false
                                );
                                detail::CheckDeadlineNotExpired(deadline);
                                if (!SQL_SUCCEEDED(prepare_result)) {
                                    throw MakeDriverError<StatementError>(
                                        "Failed to prepare ODBC query",
                                        prepare_result,
                                        statement_handle.get(),
                                        SQL_HANDLE_STMT
                                    );
                                }
                            }

                            auto result = ExecutePreparedStatement(
                                statement_handle.get(),
                                parameters,
                                deadline,
                                execution_started,
                                parameter_buffers
                            );
                            cached_metadata.row_producing = result.FieldCount() != 0;
                            if (cached_metadata.row_producing == true) {
                                // A row-producing observation supersedes any
                                // stale cross-mode bulk-DML validation.
                                cached_metadata.bulk_dml_validated = false;
                            }
                            if (!IsInsideTransaction() && InvalidatesPreparedStatements(SQL_COMMIT)) {
                                AccountPreparedStatementFailure(was_cached);
                                ClearPreparedStatements(true);
                            } else if (cache_enabled) {
                                StorePreparedStatement(
                                    query,
                                    std::move(statement_handle),
                                    was_cached,
                                    operation_cache_reset_generation,
                                    cached_metadata
                                );
                            }
                            return result;
                        } catch (...) {
                            AccountPreparedStatementFailure(was_cached);
                            if (execution_started && !IsInsideTransaction() &&
                                (InvalidatesPreparedStatements(SQL_COMMIT) ||
                                 InvalidatesPreparedStatements(SQL_ROLLBACK)))
                            {
                                ClearPreparedStatements(true);
                            }
                            throw;
                        }
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

BulkResult Connection::QueryBulk(
    const storages::odbc::Query& query,
    const impl::ParameterRows& rows,
    const detail::BulkLayout& layout,
    std::size_t chunk_rows,
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
            return RunBlockingChecked(blocking_task_processor_, deadline, [&, query_text = std::string{statement}] {
                const std::lock_guard lock{handle_mutex_};
                ApplyPreparedStatementCacheSettings();
                const auto operation_cache_reset_generation = applied_prepared_cache_reset_generation_;
                const bool cache_enabled = applied_prepared_cache_size_ != 0;
                CachedStatementMetadata handle_metadata;
                auto statement_handle =
                    cache_enabled
                        ? TakePreparedStatement(query_text, &handle_metadata)
                        : StatementHandle{nullptr, StatementHandleDeleter{this}};
                bool handle_was_cached = statement_handle != nullptr;
                bool any_execution_started = false;
                std::size_t completed_rows = 0;
                bool processed_is_known = true;
                std::optional<std::size_t> aggregate_rows_affected{0};
                std::vector<BulkRowStatus> statuses(rows.size(), BulkRowStatus::kUnused);

                const auto make_bulk_error =
                    [&](std::string message,
                        std::vector<DiagnosticRecord> diagnostics,
                        std::optional<std::size_t> processed,
                        bool invalid_handle = false) {
                        return BulkExecutionError{
                            FormatBulkErrorMessage(std::move(message), diagnostics),
                            std::move(diagnostics),
                            BulkResult{rows.size(), processed, aggregate_rows_affected, statuses},
                            invalid_handle,
                        };
                    };
                const auto account_rows_affected = [&](std::optional<std::size_t> count) {
                    if (!count || !aggregate_rows_affected) {
                        aggregate_rows_affected.reset();
                        return;
                    }
                    if (*count > std::numeric_limits<std::size_t>::max() - *aggregate_rows_affected) {
                        throw StatementError("ODBC bulk aggregate row count overflows size_t");
                    }
                    *aggregate_rows_affected += *count;
                };
                const auto discard_current_statement = [&] {
                    AccountPreparedStatementFailure(handle_was_cached);
                    handle_was_cached = false;
                    handle_metadata = {};
                    statement_handle.reset();
                };
                const auto ensure_connection_remains_usable = [&] {
                    if (IsMarkedBroken()) {
                        throw ConnectionError("ODBC statement cleanup left the connection unusable");
                    }
                };
                const auto prepare_current_statement = [&](bool must_prepare) {
                    if (!statement_handle) {
                        statement_handle = MakeStatementHandle();
                        must_prepare = true;
                        handle_metadata = {};
                    }

                    SetBulkStatementTimeout(statement_handle.get(), deadline);

                    if (must_prepare) {
                        handle_metadata = {};
                        std::vector<SQLCHAR> query_buffer(query_text.begin(), query_text.end());
                        query_buffer.push_back('\0');
                        detail::CheckDeadlineNotExpired(deadline);
                        const auto prepare_result = SQLPrepare(statement_handle.get(), query_buffer.data(), SQL_NTS);
                        HandleStatementWarnings(
                            prepare_result,
                            statement_handle.get(),
                            "preparing a bulk DML query",
                            false
                        );
                        detail::CheckDeadlineNotExpired(deadline);
                        if (!SQL_SUCCEEDED(prepare_result)) {
                            throw MakeDriverError<StatementError>(
                                "Failed to prepare ODBC bulk DML query",
                                prepare_result,
                                statement_handle.get(),
                                SQL_HANDLE_STMT
                            );
                        }
                    }
                    if (!handle_metadata.bulk_dml_validated) {
                        ValidateBulkDmlStatement(statement_handle.get(), rows.front().size(), true, deadline);
                        handle_metadata.bulk_dml_validated = true;
                    } else {
                        ValidateBulkDmlStatement(statement_handle.get(), rows.front().size(), false, deadline);
                    }
                };
                const auto rotate_after_autocommit = [&](bool has_more_rows) {
                    if (IsInsideTransaction() || !InvalidatesPreparedStatements(SQL_COMMIT)) {
                        return;
                    }
                    discard_current_statement();
                    ClearPreparedStatements(true);
                    ensure_connection_remains_usable();
                    if (has_more_rows) {
                        prepare_current_statement(true);
                    }
                };
                const auto evict_after_failure = [&] {
                    discard_current_statement();
                    if (any_execution_started && !IsInsideTransaction() &&
                        (InvalidatesPreparedStatements(SQL_COMMIT) || InvalidatesPreparedStatements(SQL_ROLLBACK)))
                    {
                        ClearPreparedStatements(true);
                    }
                };

                try {
                    prepare_current_statement(!handle_was_cached || !handle_metadata.bulk_dml_validated);
                    bool native_attributes_supported = layout.native_binding_allowed;
                    const auto statuses_are_trusted =
                        driver_capabilities_.GetParameterArrayRowCounts() == detail::ParameterArrayRowCounts::kBatch;

                    while (completed_rows < rows.size()) {
                        detail::CheckDeadlineNotExpired(deadline);
                        auto count = std::min({
                            chunk_rows,
                            detail::kMaxNativeBulkRows,
                            rows.size() - completed_rows,
                        });
                        std::optional<detail::BulkBindings> bindings;
                        if (native_attributes_supported && count > 1) {
                            while (count > 1) {
                                bindings = detail::TryBuildBulkBindings(rows, layout, completed_rows, count);
                                if (bindings) {
                                    break;
                                }
                                count = std::max<std::size_t>(1, count / 2);
                            }
                        }

                        bool use_scalar = !native_attributes_supported || count == 1 || !bindings;
                        if (!use_scalar) {
                            auto& native = *bindings;
                            bool cleanup_done = false;
                            try {
                                const auto attribute_setup =
                                    ConfigureNativeBulkAttributes(statement_handle.get(), native, deadline);
                                if (attribute_setup == BulkAttributeSetup::kFallback) {
                                    try {
                                        CleanupBulkStatement(statement_handle.get());
                                    } catch (const std::exception& cleanup_error) {
                                        LOG_WARNING()
                                            << "Discarding an ODBC statement after unsupported bulk attributes and "
                                               "uncertain reset: "
                                            << cleanup_error;
                                    }
                                    cleanup_done = true;
                                    discard_current_statement();
                                    ensure_connection_remains_usable();
                                    native_attributes_supported = false;
                                    prepare_current_statement(true);
                                    use_scalar = true;
                                } else {
                                    BindNativeBulkParameters(statement_handle.get(), native, deadline);
                                    std::fill_n(
                                        statuses.begin() + static_cast<std::ptrdiff_t>(completed_rows),
                                        count,
                                        BulkRowStatus::kUnknown
                                    );
                                    SetBulkStatementTimeout(statement_handle.get(), deadline);
                                    detail::CheckDeadlineNotExpired(deadline);
                                    any_execution_started = true;
                                    const auto execute_result = SQLExecute(statement_handle.get());
                                    auto diagnostics =
                                        execute_result == SQL_SUCCESS
                                            ? std::vector<DiagnosticRecord>{}
                                            : detail::GetSQLDiagnostics(statement_handle.get(), SQL_HANDLE_STMT);
                                    detail::CheckDeadlineNotExpired(deadline);

                                    std::optional<std::size_t> chunk_processed;
                                    bool processed_protocol_error = false;
                                    if (native.processed != detail::kBulkProcessedUntouched) {
                                        if (native.processed > count) {
                                            processed_protocol_error = true;
                                        } else {
                                            chunk_processed = static_cast<std::size_t>(native.processed);
                                        }
                                    }
                                    const auto status_decision = detail::EvaluateBulkChunkStatuses(
                                        native.statuses,
                                        count,
                                        chunk_processed,
                                        statuses_are_trusted,
                                        execute_result
                                    );
                                    std::copy(
                                        status_decision.statuses.begin(),
                                        status_decision.statuses.end(),
                                        statuses.begin() + static_cast<std::ptrdiff_t>(completed_rows)
                                    );

                                    if (!SQL_SUCCEEDED(execute_result) && execute_result != SQL_NO_DATA) {
                                        aggregate_rows_affected.reset();
                                        cleanup_done = true;
                                        auto error = make_bulk_error(
                                            "Failed to execute ODBC native bulk DML",
                                            std::move(diagnostics),
                                            std::nullopt,
                                            execute_result == SQL_INVALID_HANDLE
                                        );
                                        try {
                                            CleanupBulkStatement(statement_handle.get());
                                        } catch (const std::exception& cleanup_error) {
                                            LOG_WARNING()
                                                << "Failed to reset an ODBC statement after native bulk execution "
                                                   "failed: "
                                                << cleanup_error;
                                            discard_current_statement();
                                        } catch (...) {
                                            LOG_WARNING()
                                                << "Failed to reset an ODBC statement after native bulk execution "
                                                   "failed";
                                            discard_current_statement();
                                        }
                                        throw error;
                                    }

                                    const bool
                                        row_status_failure = processed_protocol_error || status_decision.is_failure;
                                    const bool diagnostic_failure =
                                        execute_result == SQL_SUCCESS_WITH_INFO &&
                                        !HasOnlyWarningClassDiagnostics(diagnostics);
                                    if (row_status_failure || diagnostic_failure) {
                                        const auto processed =
                                            processed_is_known && chunk_processed
                                                ? std::optional<std::size_t>{completed_rows + *chunk_processed}
                                                : std::nullopt;
                                        const auto error_message =
                                            row_status_failure
                                                ? "ODBC driver reported a failed or indeterminate native bulk row"
                                                : "ODBC native bulk DML returned error-class diagnostics";

                                        // The execution failure is primary. Drain and reset are
                                        // still mandatory while the bound buffers are alive, but
                                        // their errors must not replace row-status/diagnostic truth.
                                        try {
                                            const auto drained = DrainBulkDml(statement_handle.get(), deadline);
                                            account_rows_affected(drained.rows_affected);
                                        } catch (const OperationInterrupted& drain_error) {
                                            NotifyBroken();
                                            aggregate_rows_affected.reset();
                                            LOG_WARNING()
                                                << "Timed out while draining ODBC bulk DML after a primary row "
                                                   "failure: "
                                                << drain_error;
                                        } catch (const std::exception& drain_error) {
                                            aggregate_rows_affected.reset();
                                            LOG_WARNING()
                                                << "Failed to drain ODBC bulk DML after a primary row failure: "
                                                << drain_error;
                                        } catch (...) {
                                            aggregate_rows_affected.reset();
                                            LOG_WARNING()
                                                << "Failed to drain ODBC bulk DML after a primary row "
                                                   "failure";
                                        }

                                        cleanup_done = true;
                                        try {
                                            CleanupBulkStatement(statement_handle.get());
                                        } catch (const std::exception& cleanup_error) {
                                            LOG_WARNING()
                                                << "Failed to reset ODBC bulk DML after a primary row failure: "
                                                << cleanup_error;
                                            discard_current_statement();
                                        } catch (...) {
                                            LOG_WARNING()
                                                << "Failed to reset ODBC bulk DML after a primary row "
                                                   "failure";
                                            discard_current_statement();
                                        }
                                        throw make_bulk_error(error_message, std::move(diagnostics), processed);
                                    }

                                    auto drained = DrainBulkDml(statement_handle.get(), deadline);
                                    account_rows_affected(drained.rows_affected);
                                    CleanupBulkStatement(statement_handle.get());
                                    cleanup_done = true;

                                    if (execute_result == SQL_SUCCESS_WITH_INFO) {
                                        LogOdbcWarnings("Executing ODBC native bulk DML", diagnostics);
                                    }
                                    if (!chunk_processed) {
                                        processed_is_known = false;
                                    }
                                    completed_rows += count;
                                    rotate_after_autocommit(completed_rows < rows.size());
                                }
                            } catch (...) {
                                const auto original_error = std::current_exception();
                                if (!cleanup_done) {
                                    bool cleanup_succeeded = false;
                                    try {
                                        CleanupBulkStatement(statement_handle.get());
                                        cleanup_succeeded = true;
                                    } catch (const std::exception& cleanup_error) {
                                        LOG_WARNING()
                                            << "Failed to clean up an erroneous native ODBC bulk statement: "
                                            << cleanup_error;
                                    } catch (...) {
                                        LOG_WARNING() << "Failed to clean up an erroneous native ODBC bulk statement";
                                    }
                                    if (!cleanup_succeeded) {
                                        discard_current_statement();
                                    }
                                }
                                std::rethrow_exception(original_error);
                            }
                        }

                        if (use_scalar) {
                            bool scalar_execution_started = false;
                            statuses[completed_rows] = BulkRowStatus::kUnknown;
                            try {
                                const auto scalar = ExecuteScalarBulkDml(
                                    statement_handle.get(),
                                    rows[completed_rows],
                                    deadline,
                                    scalar_execution_started,
                                    discard_current_statement
                                );
                                any_execution_started = any_execution_started || scalar_execution_started;
                                statuses[completed_rows] = scalar.status;
                                account_rows_affected(scalar.rows_affected);
                            } catch (const StatementError& error) {
                                any_execution_started = any_execution_started || scalar_execution_started;
                                if (scalar_execution_started) {
                                    statuses[completed_rows] = BulkRowStatus::kError;
                                    aggregate_rows_affected.reset();
                                    throw make_bulk_error(
                                        "Failed to execute an ODBC scalar bulk fallback row",
                                        error.GetDiagnostics(),
                                        processed_is_known
                                            ? std::optional<std::size_t>{completed_rows + 1}
                                            : std::nullopt,
                                        error.IsInvalidHandle()
                                    );
                                }
                                if (completed_rows != 0) {
                                    statuses[completed_rows] = BulkRowStatus::kError;
                                    throw make_bulk_error(
                                        "Failed to bind an ODBC scalar bulk fallback row",
                                        error.GetDiagnostics(),
                                        processed_is_known ? std::optional<std::size_t>{completed_rows} : std::nullopt,
                                        error.IsInvalidHandle()
                                    );
                                }
                                throw;
                            }
                            ++completed_rows;
                            rotate_after_autocommit(completed_rows < rows.size());
                        }
                    }

                    if (cache_enabled && statement_handle) {
                        handle_metadata.row_producing = false;
                        StorePreparedStatement(
                            query_text,
                            std::move(statement_handle),
                            handle_was_cached,
                            operation_cache_reset_generation,
                            handle_metadata
                        );
                    }
                    return BulkResult{
                        rows.size(),
                        processed_is_known ? std::optional<std::size_t>{rows.size()} : std::nullopt,
                        aggregate_rows_affected,
                        std::move(statuses),
                    };
                } catch (const BulkExecutionError&) {
                    evict_after_failure();
                    throw;
                } catch (const StatementError& error) {
                    evict_after_failure();
                    if (any_execution_started) {
                        aggregate_rows_affected.reset();
                        throw make_bulk_error(
                            "ODBC bulk DML failed after execution started",
                            error.GetDiagnostics(),
                            std::nullopt,
                            error.IsInvalidHandle()
                        );
                    }
                    throw;
                } catch (...) {
                    evict_after_failure();
                    throw;
                }
            });
        });
    } catch (const OperationInterrupted&) {
        NotifyBroken();
        throw;
    } catch (const StatementError& ex) {
        if (ex.IsInvalidHandle() || detail::HasConnectionError(ex.GetDiagnostics())) {
            NotifyBroken();
        } else {
            UpdateBrokenFromDriver();
        }
        span.AddTag(tracing::kErrorFlag, true);
        span.AddTag(tracing::kErrorMessage, ex.what());
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
            ApplyPreparedStatementCacheSettings();
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
                        if (attempted_restore.autocommit && InvalidatesPreparedStatements(SQL_COMMIT)) {
                            ClearPreparedStatements(true);
                        }
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

    ApplyPreparedStatementCacheSettings();
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

    if (InvalidatesPreparedStatements(completion_type)) {
        ClearPreparedStatements(true);
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
