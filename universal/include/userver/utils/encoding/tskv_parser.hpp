#pragma once

/// @file userver/utils/encoding/tskv_parser.hpp
/// @brief @copybrief utils::encoding::TskvParser

#include <optional>
#include <string>
#include <string_view>

USERVER_NAMESPACE_BEGIN

namespace utils::encoding {

/// @brief A streaming parser for the TSKV variant that userver emits.
///
/// **Supported syntax**
/// 1. Records are separated by a single non-escaped `\n` character
/// 2. A record consists of entries separated by a single `\t` character
/// 3. The first entry SHOULD be verbatim `tskv`
/// 4. Entries MAY have values, separated by the first non-escaped `=`
/// 5. An entry MAY have no key-value split, in which case it all counts as key
///
/// **Escaping**
/// 1. `\n`, `\t`, `\\` in keys or values SHOULD be escaped as
///    `"\\\n"`, `"\\\t"`, `"\\\\"`
/// 2. `=` SHOULD be escaped as `"\\="` in keys, MAY be escaped in values
/// 3. `\r` and `\0` MAY be escaped as `"\\\r"`, `"\\\0"`
///
/// **Parsing process**
/// Initialize `TskvParser` with a caller-owned string that may contain a single
/// or multiple TSKV records. For each record, first call `SkipToRecordBegin`.
/// Then call `ReadKey` and `ReadValue` a bunch of times.
///
/// For a simpler way to read TSKV records, see utils::encoding::TskvReadRecord.
class TskvParser final {
public:
    /// The status is returned once the record is complete. We ignore invalid
    /// escaping, not reporting it as an error, trying to push through.
    /// `nullopt` status means we should keep reading the current TSKV record.
    enum class [[nodiscard]] RecordStatus {
        kReachedEnd,  ///< successfully read the whole record
        kIncomplete,  ///< the record ends abruptly, the service is probably
                      ///< still writing the record
    };

    explicit TskvParser(std::string_view in) noexcept;

    /// @brief Skips the current record or optional invalid junk until it finds
    /// the start of the a valid record.
    /// @returns pointer to the start of the next record if there is one,
    /// `nullptr` otherwise
    /// @note `tskv\n` records are currently not supported
    const char* SkipToRecordBegin() noexcept;

    /// @brief Skips to the end of the current record, which might not be the same
    /// as the start of the next valid record.
    /// @note RecordStatus::kReachedEnd SHOULD
    /// not have been returned for the current record, otherwise the behavior is
    /// unspecified.
    /// @note `tskv\n` records are currently not supported
    RecordStatus SkipToRecordEnd() noexcept;

    /// @brief Parses the key, replacing `result` with it.
    /// @throws std::bad_alloc only if `std::string` allocation throws
    /// @note RecordStatus::kReachedEnd is returned specifically
    /// on a trailing `\t\n` sequence
    std::optional<RecordStatus> ReadKey(std::string& result);

    /// @brief Parses the value, replacing `result` with it.
    /// @throws std::bad_alloc only if `std::string` allocation throws
    std::optional<RecordStatus> ReadValue(std::string& result);

    /// @returns pointer to the data that will be read by the next operation
    const char* GetStreamPosition() const noexcept;

private:
    std::string_view in_;
};

}  // namespace utils::encoding

USERVER_NAMESPACE_END
