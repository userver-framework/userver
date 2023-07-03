#pragma once

#include <stdexcept>
#include <string_view>
#include <type_traits>

#include <userver/logging/log_extra.hpp>
#include <userver/logging/log_helper.hpp>
#include <userver/utils/encoding/tskv.hpp>
#include <userver/utils/internal_tag_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging::impl {

// Allows to add tags directly to the logged message, bypassing LogExtra.
class TagWriter {
 public:
  template <typename T>
  void PutTag(std::string_view key, const T& value);

  void PutLogExtra(const LogExtra& extra);

  explicit TagWriter(utils::InternalTag, LogHelper& lh) noexcept;

 private:
  LogHelper& lh_;
};

template <typename T>
void TagWriter::PutTag(std::string_view key, const T& value) {
  lh_.Put(utils::encoding::kTskvPairsSeparator);
  {
    const logging::LogHelper::EncodingGuard guard{
        lh_, logging::LogHelper::Encode::kKeyReplacePeriod};
    lh_.Put(key);
  }
  lh_.PutKeyValueSeparator();
  lh_ << value;
}

}  // namespace logging::impl

USERVER_NAMESPACE_END
