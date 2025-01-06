#pragma once

/// @file userver/utils/encoding/tskv_parser_read.hpp
/// @brief @copybrief utils::encoding::TskvReadKeysValues

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

/// @brief Read all keys-values for 1 TSKV record.
///
/// @param parser parser that should have already found the start of the TSKV
/// record using TskvParser::SkipToRecordBegin
/// @param consumer a lambda with the signature
/// `(const std::string&, const std::string&) -> bool`;
/// the strings are temporaries, references to them should not be stored;
/// can return `false` to skip the record and return immediately
///
/// Usage example:
/// @snippet utils/encoding/tskv_parser_test.cpp  sample
template <typename TagConsumer>
TskvParser::RecordStatus TskvReadRecord(TskvParser& parser, TagConsumer consumer) {
    using RecordStatus = TskvParser::RecordStatus;

    auto kv_storage = impl::tskv_parser_kv_storage.Use();

    while (true) {
        if (const auto key_status = parser.ReadKey(kv_storage->key)) {
            return *key_status;
        }

        const auto value_status = parser.ReadValue(kv_storage->value);
        if (value_status == RecordStatus::kIncomplete) {
            return RecordStatus::kIncomplete;
        }

        const bool record_ok = consumer(std::as_const(kv_storage->key), std::as_const(kv_storage->value));
        if (value_status == RecordStatus::kReachedEnd) {
            return RecordStatus::kReachedEnd;
        }
        if (!record_ok) {
            return parser.SkipToRecordEnd();
        }
    }
}

}  // namespace utils::encoding

USERVER_NAMESPACE_END
