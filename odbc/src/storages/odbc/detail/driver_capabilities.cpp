#include <storages/odbc/detail/driver_capabilities.hpp>

#include <algorithm>
#include <limits>
#include <string_view>
#include <utility>
#include <vector>

#include <fmt/format.h>

#include <storages/odbc/detail/diag_wrapper.hpp>
#include <userver/logging/log.hpp>
#include <userver/storages/odbc/exception.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc::detail {

namespace {

constexpr std::size_t kInitialStringBufferSize = 256;
constexpr std::size_t kMaxWarningLength = 1024;

enum class Requirement { kRequired, kOptional };

bool HasSqlState(const std::vector<DiagnosticRecord>& diagnostics, std::string_view state) {
    return std::any_of(diagnostics.begin(), diagnostics.end(), [state](const DiagnosticRecord& diagnostic) {
        return diagnostic.sql_state == state;
    });
}

bool IsUnsupportedInfo(const std::vector<DiagnosticRecord>& diagnostics) {
    return !diagnostics.empty() &&
           std::all_of(diagnostics.begin(), diagnostics.end(), [](const DiagnosticRecord& diagnostic) {
               return diagnostic.sql_state == "HYC00" || diagnostic.sql_state == "HY096";
           });
}

void LogWarnings(std::string_view info_name, const std::vector<DiagnosticRecord>& diagnostics) {
    auto formatted = FormatSQLDiagnostics(diagnostics);
    if (formatted.empty()) {
        formatted = "no diagnostic records";
    } else if (formatted.size() > kMaxWarningLength) {
        formatted.resize(kMaxWarningLength);
        formatted += "...";
    }
    LOG_WARNING() << "ODBC SQLGetInfo(" << info_name << ") completed with warning: " << formatted;
}

[[noreturn]] void ThrowGetInfoError(
    std::string_view info_name,
    SQLRETURN result,
    std::vector<DiagnosticRecord> diagnostics
) {
    auto message = fmt::format("Failed to read ODBC {} with SQLGetInfo", info_name);
    const auto formatted = FormatSQLDiagnostics(diagnostics);
    if (!formatted.empty()) {
        message += ": ";
        message += formatted;
    }
    throw ConnectionError{std::move(message), std::move(diagnostics), result == SQL_INVALID_HANDLE};
}

[[noreturn]] void ThrowInvalidInfoValue(std::string_view info_name, std::string_view value) {
    throw ConnectionError{
        fmt::format("ODBC SQLGetInfo returned invalid {} value '{}'", info_name, value),
        std::vector<DiagnosticRecord>{},
    };
}

class InfoReader final {
public:
    InfoReader(SQLHDBC connection, engine::Deadline deadline)
        : connection_{connection},
          deadline_{deadline}
    {}

    std::string ReadRequiredString(SQLUSMALLINT info_type, std::string_view info_name) const {
        auto value = ReadString(info_type, info_name, Requirement::kRequired);
        if (!value || value->empty()) {
            ThrowInvalidInfoValue(info_name, value ? std::string_view{*value} : std::string_view{"<unsupported>"});
        }
        return std::move(*value);
    }

    std::optional<bool> ReadOptionalYesNo(SQLUSMALLINT info_type, std::string_view info_name) const {
        const auto value = ReadString(info_type, info_name, Requirement::kOptional);
        if (!value) {
            return std::nullopt;
        }
        if (*value == "Y") {
            return true;
        }
        if (*value == "N") {
            return false;
        }
        ThrowInvalidInfoValue(info_name, *value);
    }

    template <typename T>
    std::optional<T> ReadOptionalNumber(SQLUSMALLINT info_type, std::string_view info_name) const {
        CheckDeadline();
        T value{};
        const auto
            result = SQLGetInfo(connection_, info_type, &value, static_cast<SQLSMALLINT>(sizeof(value)), nullptr);
        CheckDeadline();
        auto diagnostics =
            result == SQL_SUCCESS ? std::vector<DiagnosticRecord>{} : GetSQLDiagnostics(connection_, SQL_HANDLE_DBC);

        if (!SQL_SUCCEEDED(result)) {
            if (IsUnsupportedInfo(diagnostics)) {
                return std::nullopt;
            }
            ThrowGetInfoError(info_name, result, std::move(diagnostics));
        }
        if (HasSqlState(diagnostics, "01004")) {
            // An exact-size numeric result cannot be usefully recovered by retrying.
            ThrowGetInfoError(info_name, result, std::move(diagnostics));
        }
        if (result == SQL_SUCCESS_WITH_INFO) {
            LogWarnings(info_name, diagnostics);
        }
        return value;
    }

private:
    void CheckDeadline() const {
        if (deadline_.IsReachable() && deadline_.IsReached()) {
            throw OperationInterrupted("Cancelled by deadline");
        }
    }

    std::optional<std::string> ReadString(SQLUSMALLINT info_type, std::string_view info_name, Requirement requirement)
        const {
        constexpr auto kMaxBufferSize = static_cast<std::size_t>(std::numeric_limits<SQLSMALLINT>::max());
        std::vector<SQLCHAR> buffer(kInitialStringBufferSize);

        while (true) {
            CheckDeadline();
            SQLSMALLINT length = 0;
            const auto result =
                SQLGetInfo(connection_, info_type, buffer.data(), static_cast<SQLSMALLINT>(buffer.size()), &length);
            CheckDeadline();
            auto diagnostics =
                result == SQL_SUCCESS
                    ? std::vector<DiagnosticRecord>{}
                    : GetSQLDiagnostics(connection_, SQL_HANDLE_DBC);

            if (!SQL_SUCCEEDED(result)) {
                if (requirement == Requirement::kOptional && IsUnsupportedInfo(diagnostics)) {
                    return std::nullopt;
                }
                ThrowGetInfoError(info_name, result, std::move(diagnostics));
            }
            if (length < 0) {
                ThrowGetInfoError(info_name, result, std::move(diagnostics));
            }

            const auto value_size = static_cast<std::size_t>(length);
            const bool truncated = HasSqlState(diagnostics, "01004") || value_size >= buffer.size();
            if (truncated) {
                if (result == SQL_SUCCESS_WITH_INFO) {
                    auto non_truncation_diagnostics = diagnostics;
                    non_truncation_diagnostics.erase(
                        std::remove_if(
                            non_truncation_diagnostics.begin(),
                            non_truncation_diagnostics.end(),
                            [](const DiagnosticRecord& diagnostic) { return diagnostic.sql_state == "01004"; }
                        ),
                        non_truncation_diagnostics.end()
                    );
                    if (!non_truncation_diagnostics.empty()) {
                        LogWarnings(info_name, non_truncation_diagnostics);
                    }
                }

                const auto reported_buffer_size = value_size < kMaxBufferSize ? value_size + 1 : kMaxBufferSize;
                const auto doubled_buffer_size = std::min(kMaxBufferSize, buffer.size() * 2);
                const auto next_buffer_size = std::max(reported_buffer_size, doubled_buffer_size);
                if (next_buffer_size <= buffer.size()) {
                    ThrowGetInfoError(info_name, result, std::move(diagnostics));
                }
                buffer.resize(next_buffer_size);
                continue;
            }

            if (result == SQL_SUCCESS_WITH_INFO) {
                LogWarnings(info_name, diagnostics);
            }
            return std::string{reinterpret_cast<const char*>(buffer.data()), value_size};
        }
    }

    SQLHDBC connection_;
    engine::Deadline deadline_;
};

std::optional<TransactionCapability> ReadTransactionCapability(const InfoReader& reader) {
    const auto value = reader.ReadOptionalNumber<SQLUSMALLINT>(SQL_TXN_CAPABLE, "SQL_TXN_CAPABLE");
    if (!value) {
        return std::nullopt;
    }
    switch (*value) {
        case SQL_TC_NONE:
            return TransactionCapability::kNone;
        case SQL_TC_DML:
            return TransactionCapability::kDml;
        case SQL_TC_ALL:
            return TransactionCapability::kAll;
        case SQL_TC_DDL_COMMIT:
            return TransactionCapability::kDdlCommit;
        case SQL_TC_DDL_IGNORE:
            return TransactionCapability::kDdlIgnore;
        default:
            ThrowInvalidInfoValue("SQL_TXN_CAPABLE", fmt::format("{}", *value));
    }
}

std::optional<ParameterArrayRowCounts> ReadParameterArrayRowCounts(const InfoReader& reader) {
    const auto value = reader.ReadOptionalNumber<SQLUINTEGER>(SQL_PARAM_ARRAY_ROW_COUNTS, "SQL_PARAM_ARRAY_ROW_COUNTS");
    if (!value) {
        return std::nullopt;
    }
    switch (*value) {
        case SQL_PARC_BATCH:
            return ParameterArrayRowCounts::kBatch;
        case SQL_PARC_NO_BATCH:
            return ParameterArrayRowCounts::kNoBatch;
        default:
            ThrowInvalidInfoValue("SQL_PARAM_ARRAY_ROW_COUNTS", fmt::format("{}", *value));
    }
}

std::optional<ParameterArraySelects> ReadParameterArraySelects(const InfoReader& reader) {
    const auto value = reader.ReadOptionalNumber<SQLUINTEGER>(SQL_PARAM_ARRAY_SELECTS, "SQL_PARAM_ARRAY_SELECTS");
    if (!value) {
        return std::nullopt;
    }
    switch (*value) {
        case SQL_PAS_BATCH:
            return ParameterArraySelects::kBatch;
        case SQL_PAS_NO_BATCH:
            return ParameterArraySelects::kNoBatch;
        case SQL_PAS_NO_SELECT:
            return ParameterArraySelects::kNoSelect;
        default:
            ThrowInvalidInfoValue("SQL_PARAM_ARRAY_SELECTS", fmt::format("{}", *value));
    }
}

std::optional<CursorBehavior> ReadCursorBehavior(
    const InfoReader& reader,
    SQLUSMALLINT info_type,
    std::string_view info_name
) {
    const auto value = reader.ReadOptionalNumber<SQLUSMALLINT>(info_type, info_name);
    if (!value) {
        return std::nullopt;
    }
    switch (*value) {
        case SQL_CB_DELETE:
            return CursorBehavior::kDelete;
        case SQL_CB_CLOSE:
            return CursorBehavior::kClose;
        case SQL_CB_PRESERVE:
            return CursorBehavior::kPreserve;
        default:
            ThrowInvalidInfoValue(info_name, fmt::format("{}", *value));
    }
}

}  // namespace

DriverCapabilities DriverCapabilities::Read(SQLHDBC connection, engine::Deadline deadline) {
    const InfoReader reader{connection, deadline};
    DriverCapabilities capabilities;

    capabilities.dbms_name_ = reader.ReadRequiredString(SQL_DBMS_NAME, "SQL_DBMS_NAME");
    capabilities.dbms_version_ = reader.ReadRequiredString(SQL_DBMS_VER, "SQL_DBMS_VER");
    capabilities.driver_name_ = reader.ReadRequiredString(SQL_DRIVER_NAME, "SQL_DRIVER_NAME");
    capabilities.driver_version_ = reader.ReadRequiredString(SQL_DRIVER_VER, "SQL_DRIVER_VER");
    capabilities.driver_odbc_version_ = reader.ReadRequiredString(SQL_DRIVER_ODBC_VER, "SQL_DRIVER_ODBC_VER");

    capabilities.transaction_capability_ = ReadTransactionCapability(reader);
    capabilities.transaction_isolation_options_ = reader.ReadOptionalNumber<
        SQLUINTEGER>(SQL_TXN_ISOLATION_OPTION, "SQL_TXN_ISOLATION_OPTION");
    capabilities.default_transaction_isolation_ = reader.ReadOptionalNumber<
        SQLUINTEGER>(SQL_DEFAULT_TXN_ISOLATION, "SQL_DEFAULT_TXN_ISOLATION");
    capabilities
        .data_source_read_only_ = reader.ReadOptionalYesNo(SQL_DATA_SOURCE_READ_ONLY, "SQL_DATA_SOURCE_READ_ONLY");
    capabilities.describe_parameter_ = reader.ReadOptionalYesNo(SQL_DESCRIBE_PARAMETER, "SQL_DESCRIBE_PARAMETER");

    capabilities.parameter_array_row_counts_ = ReadParameterArrayRowCounts(reader);
    capabilities.parameter_array_selects_ = ReadParameterArraySelects(reader);
    capabilities.batch_row_count_ = reader.ReadOptionalNumber<SQLUINTEGER>(SQL_BATCH_ROW_COUNT, "SQL_BATCH_ROW_COUNT");

    capabilities.scroll_options_ = reader.ReadOptionalNumber<SQLUINTEGER>(SQL_SCROLL_OPTIONS, "SQL_SCROLL_OPTIONS");
    capabilities
        .getdata_extensions_ = reader.ReadOptionalNumber<SQLUINTEGER>(SQL_GETDATA_EXTENSIONS, "SQL_GETDATA_EXTENSIONS");
    capabilities
        .cursor_commit_behavior_ = ReadCursorBehavior(reader, SQL_CURSOR_COMMIT_BEHAVIOR, "SQL_CURSOR_COMMIT_BEHAVIOR");
    capabilities.cursor_rollback_behavior_ =
        ReadCursorBehavior(reader, SQL_CURSOR_ROLLBACK_BEHAVIOR, "SQL_CURSOR_ROLLBACK_BEHAVIOR");

    return capabilities;
}

}  // namespace storages::odbc::detail

USERVER_NAMESPACE_END
