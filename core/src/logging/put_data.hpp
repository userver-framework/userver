#pragma once

#include <logging/log_helper_impl.hpp>
#include <userver/logging/log_helper.hpp>
#include <userver/utils/encoding/hex.hpp>
#include <userver/utils/encoding/tskv.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging {

template <typename T>
void PutData(LogHelper& lh, std::string_view key, const T& value) {
  lh.Put(utils::encoding::kTskvPairsSeparator);
  {
    logging::LogHelper::EncodingGuard guard{
        lh, logging::LogHelper::Encode::kKeyReplacePeriod};
    lh.Put(key);
  }
  lh.pimpl_->PutKeyValueSeparator();
  lh << value;
}

}  // namespace logging

USERVER_NAMESPACE_END
