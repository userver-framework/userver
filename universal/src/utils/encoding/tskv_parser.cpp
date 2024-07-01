#include <userver/utils/encoding/tskv_parser.hpp>

#include <optional>

#include <userver/utils/assert.hpp>
#include <userver/utils/text_light.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::encoding {

namespace {

constexpr std::string_view kTskv = "tskv\t";

std::optional<char> GetUnescapedChar(char second_char) {
  switch (second_char) {
    case 'n':
      return '\n';
    case 'r':
      return '\r';
    case 't':
      return '\t';
    case '0':
      return '\0';
    case '\\':
      return '\\';
    case '=':
      return '=';
    default:
      return std::nullopt;
  }
}

// Returns the number of chars consumed.
std::size_t Unescape(char second_char, std::string& result) {
  if (const auto unescaped = GetUnescapedChar(second_char)) {
    result.push_back(*unescaped);
    return 2;
  }
  // Unsupported escaping. Consider this as a garbage '\\' to avoid the
  // situation where invalid escaping contaminates the entire log.
  result.push_back('\\');
  return 1;
}

}  // namespace

TskvParser::TskvParser(std::string_view in) noexcept : in_(in) {}

const char* TskvParser::SkipToRecordBegin() noexcept {
  // TODO support empty records of the form "tskv\n"?

  while (true) {
    if (text::StartsWith(in_, kTskv)) {
      // Happy case.
      const char* const result = in_.data();
      in_.remove_prefix(kTskv.size());
      return result;
    }

    const auto newline_pos = in_.find('\n');
    if (newline_pos == std::string_view::npos) {
      return nullptr;
    }

    in_.remove_prefix(newline_pos + 1);
  }
}

TskvParser::RecordStatus TskvParser::SkipToRecordEnd() noexcept {
  const auto newline_pos = in_.find('\n');
  if (newline_pos == std::string_view::npos) {
    return RecordStatus::kIncomplete;
  }

  in_.remove_prefix(newline_pos + 1);
  return RecordStatus::kReachedEnd;
}

std::optional<TskvParser::RecordStatus> TskvParser::ReadKey(
    std::string& result) {
  result.clear();

  while (true) {
    const auto key_end = in_.find_first_of("\\=\t\n");
    if (key_end == std::string_view::npos) {
      return RecordStatus::kIncomplete;
    }
    const auto separator = in_[key_end];

    if (separator == '=') {
      result.append(in_.substr(0, key_end));
      in_.remove_prefix(key_end + 1);
      return std::nullopt;
    }

    if (separator == '\n' && key_end == 0) {
      // Drop an otherwise empty record in a "\t\n" sequence.
      in_.remove_prefix(1);
      return RecordStatus::kReachedEnd;
    }

    if (separator == '\t' || separator == '\n') {
      // A key-only entry.
      result.append(in_.substr(0, key_end));
      in_.remove_prefix(key_end);
      return std::nullopt;
    }

    UASSERT(separator == '\\');
    if (key_end + 1 == in_.size()) {
      return RecordStatus::kIncomplete;
    }
    result.append(in_.substr(0, key_end));
    in_.remove_prefix(key_end + Unescape(in_[key_end + 1], result));
  }
}

std::optional<TskvParser::RecordStatus> TskvParser::ReadValue(
    std::string& result) {
  result.clear();

  while (true) {
    const auto value_end = in_.find_first_of("\\\t\n");
    if (value_end == std::string_view::npos) {
      return RecordStatus::kIncomplete;
    }
    const auto separator = in_[value_end];

    if (separator != '\\') {
      UASSERT(separator == '\t' || separator == '\n');
      result.append(in_.substr(0, value_end));
      in_.remove_prefix(value_end + 1);
      return separator == '\n' ? std::make_optional(RecordStatus::kReachedEnd)
                               : std::nullopt;
    }

    UASSERT(separator == '\\');
    if (value_end + 1 == in_.size()) {
      return RecordStatus::kIncomplete;
    }
    result.append(in_.substr(0, value_end));
    in_.remove_prefix(value_end + Unescape(in_[value_end + 1], result));
  }
}

const char* TskvParser::GetStreamPosition() const noexcept {
  return in_.data();
}

}  // namespace utils::encoding

USERVER_NAMESPACE_END
