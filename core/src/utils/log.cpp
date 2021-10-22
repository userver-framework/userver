#include <userver/utils/log.hpp>

#include <fmt/compile.h>
#include <fmt/format.h>

#include <userver/utils/encoding/hex.hpp>
#include <userver/utils/text.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::log {

std::string ToLimitedHex(std::string_view data, size_t limit) {
  // string of length X will produce hex of length 2*X
  limit /= 2;

  if (data.size() <= limit) {
    return utils::encoding::ToHex(data);
  } else {
    auto truncated = utils::encoding::ToHex(data.data(), limit);
    return fmt::format(FMT_COMPILE("{}...(truncated, total {} bytes)"),
                       std::string_view{truncated}, data.size());
  }
}

std::string ToLimitedUtf8(std::string_view data, size_t limit) {
  if (data.size() <= limit) {
    if (utils::text::IsUtf8(data)) {
      return std::string{data};
    } else {
      return fmt::format(FMT_COMPILE("<Non utf-8, total {} bytes>"),
                         data.size());
    }
  }

  auto view = std::string_view{data.data(), limit};
  utils::text::utf8::TrimViewTruncatedEnding(view);
  if (utils::text::IsUtf8(view)) {
    return fmt::format(FMT_COMPILE("{}...(truncated, total {} bytes)"), view,
                       data.size());
  } else {
    return fmt::format(FMT_COMPILE("<Non utf-8, total {} bytes>"), data.size());
  }
}

}  // namespace utils::log

USERVER_NAMESPACE_END
