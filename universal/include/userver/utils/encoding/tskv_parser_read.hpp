#pragma once

/// @file userver/utils/encoding/tskv_parser_read.hpp
/// @brief @copybrief utils::encoding::TskvReadRecord

#include <string>
#include <string_view>
#include <utility>

#include <userver/compiler/thread_local.hpp>
#include <userver/utils/encoding/tskv_parser.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::encoding {

namespace impl {

struct TskvParserKvStorage {
    std::string key{};
    std::string value{};
};

inline compiler::ThreadLocal tskv_parser_kv_storage = [] { return TskvParserKvStorage{}; };

}  // namespace impl

/// @brief Reads selected values of one TSKV record, borrowing unescaped values.
/// @param parser parser that has found the record's start using
/// @ref utils::encoding::TskvParser::SkipToRecordBegin
/// @param select predicate `(std::string_view key) -> bool`, called for every
/// decoded key; unselected values are skipped without decoding or copying
/// @param consumer callback `(std::string_view key, std::string_view value) -> bool`;
/// false skips the remaining record
/// @note Neither callback may retain the views beyond its invocation.
/// Keys and escaped values use reusable scratch buffers. Unescaped values
/// borrow the parser's input. Copy any values that must survive the callback.
/// @note Calling this function or @ref utils::encoding::TskvReadRecord
/// from either function's callbacks on the same thread is undefined behavior,
/// even with a different parser.
///
/// Usage example:
/// @snippet universal/src/utils/encoding/tskv_parser_test.cpp sample_selected
template <typename KeyPredicate, typename TagConsumer>
TskvParser::RecordStatus TskvReadRecordSelected(TskvParser& parser, KeyPredicate select, TagConsumer consumer) {
    using RecordStatus = TskvParser::RecordStatus;

    auto kv_storage = impl::tskv_parser_kv_storage.Use();

    while (true) {
        if (const auto key_status = parser.ReadKey(kv_storage->key)) {
            return *key_status;
        }

        const bool selected = select(std::string_view{kv_storage->key});
        std::string_view value;
        const auto value_status = selected ? parser.ReadValueView(value, kv_storage->value) : parser.SkipValue();
        if (value_status == RecordStatus::kIncomplete) {
            return RecordStatus::kIncomplete;
        }

        const bool record_ok = !selected || consumer(std::string_view{kv_storage->key}, value);
        if (value_status == RecordStatus::kReachedEnd) {
            return RecordStatus::kReachedEnd;
        }
        if (!record_ok) {
            return parser.SkipToRecordEnd();
        }
    }
}

/// @brief Read all keys-values for 1 TSKV record.
///
/// @param parser parser that should have already found the start of the TSKV
/// record using @ref utils::encoding::TskvParser::SkipToRecordBegin
/// @param consumer callback `(std::string_view key, std::string_view value) -> bool`;
/// false skips the remaining record
/// @note Views are valid only for the duration of the callback.
/// Copy any keys or values that must survive it.
/// @note Calling this function or @ref utils::encoding::TskvReadRecordSelected
/// from either function's callbacks on the same thread is undefined behavior,
/// even with a different parser.
///
/// Usage example:
/// @snippet universal/src/utils/encoding/tskv_parser_test.cpp sample
template <typename TagConsumer>
TskvParser::RecordStatus TskvReadRecord(TskvParser& parser, TagConsumer consumer) {
    return TskvReadRecordSelected(parser, [](std::string_view) { return true; }, std::move(consumer));
}

}  // namespace utils::encoding

USERVER_NAMESPACE_END
