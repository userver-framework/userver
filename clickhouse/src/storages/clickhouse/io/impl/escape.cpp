#include <userver/storages/clickhouse/io/impl/escape.hpp>

#include <userver/storages/clickhouse/io/type_traits.hpp>

#include <fmt/format.h>

USERVER_NAMESPACE_BEGIN

namespace storages::clickhouse::io::impl {

namespace {

void EscapeSymbol(std::string& result, char c) {
  // clang-format off
  // taken from https://github.com/mymarilyn/clickhouse-driver/blob/87090902f0270ed51a0b6754d5cbf0dc8544ec4b/clickhouse_driver/util/escape.py#L8
       if (c == '\b') result += R"(\b)";
  else if (c == '\f') result += R"(\f)";
  else if (c == '\r') result += R"(\r)";
  else if (c == '\n') result += R"(\n)";
  else if (c == '\t') result += R"(\t)";
  else if (c == '\0') result += R"(\0)";
  else if (c == '\a') result += R"(\a)";
  else if (c == '\v') result += R"(\v)";
  else if (c == '\\') result += R"(\\)";
  else if (c == '\'') result += R"(\')";
  else
    result.push_back(c);
  // clang-format on
}

template <typename T>
std::string FormatScalar(T v) {
  return fmt::format("{}", v);
}

template <typename Rep>
std::string FormatDatetime64(Rep source) {
  using time_resolution = typename Rep::TagType::time_resolution;

  constexpr auto ratio = time_resolution::period::den;
  const auto tics = std::chrono::duration_cast<time_resolution>(
                        source.GetUnderlying().time_since_epoch())
                        .count();
  const size_t prefix = tics / ratio;
  const size_t suffix = tics % ratio;
  return fmt::format("toDateTime64('{}.{}', {})", prefix, suffix,
                     Rep::TagType::kPrecision);
}

}  // namespace

std::string Escape(uint8_t v) { return FormatScalar(v); }
std::string Escape(uint16_t v) { return FormatScalar(v); }
std::string Escape(uint32_t v) { return FormatScalar(v); }
std::string Escape(uint64_t v) { return FormatScalar(v); }
std::string Escape(int8_t v) { return FormatScalar(v); }
std::string Escape(int16_t v) { return FormatScalar(v); }
std::string Escape(int32_t v) { return FormatScalar(v); }
std::string Escape(int64_t v) { return FormatScalar(v); }

std::string Escape(const char* source) {
  return Escape(std::string_view{source});
}

std::string Escape(const std::string& source) {
  return Escape(std::string_view{source});
}

std::string Escape(std::string_view source) {
  std::string result;
  result.reserve(source.size() + 2);

  result.push_back('\'');
  for (auto c : source) EscapeSymbol(result, c);
  result.push_back('\'');

  return result;
}

std::string Escape(std::chrono::system_clock::time_point source) {
  return fmt::format("toDateTime({})",
                     std::chrono::duration_cast<std::chrono::seconds>(
                         source.time_since_epoch())
                         .count());
}

std::string Escape(DateTime64Milli source) { return FormatDatetime64(source); }

std::string Escape(DateTime64Micro source) { return FormatDatetime64(source); }

std::string Escape(DateTime64Nano source) { return FormatDatetime64(source); }

}  // namespace storages::clickhouse::io::impl

USERVER_NAMESPACE_END
