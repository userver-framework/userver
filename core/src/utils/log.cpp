#include <utils/log.hpp>

#include <fmt/format.h>

#include <utils/encoding/hex.hpp>
#include <utils/text.hpp>

namespace utils::log {

std::string ToLimitedHex(const std::string& data, size_t limit) {
  // string of length X will produce hex of length 2*X
  limit /= 2;

  if (data.size() <= limit) {
    return utils::encoding::ToHex(std::string_view{data});
  } else {
    auto truncated = utils::encoding::ToHex(data.data(), limit);
    return fmt::format("{}...(truncated, total {} bytes)",
                       std::string_view{truncated}, data.size());
  }
}

std::string ToLimitedUtf8(const std::string& data, size_t limit) {
  if (data.size() <= limit) {
    if (utils::text::IsUtf8(std::string_view{data})) {
      return data;
    } else {
      return fmt::format("<Non utf-8, total {} bytes>", data.size());
    }
  }

  auto view = std::string_view{data.data(), limit};
  if (utils::text::IsUtf8(view)) {
    utils::text::utf8::TrimViewTruncatedEnding(view);
    return fmt::format("{}...(truncated, total {} bytes)", view, data.size());
  } else {
    return fmt::format("<Non utf-8, total {} bytes>", data.size());
  }
}

}  // namespace utils::log
