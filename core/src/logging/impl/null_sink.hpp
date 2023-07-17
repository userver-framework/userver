#pragma once

#include <logging/impl/base_sink.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging::impl {

class NullSink final : public BaseSink {
 public:
  NullSink() = default;

 protected:
  void Write(std::string_view /*log*/) override {}
};

}  // namespace logging::impl

USERVER_NAMESPACE_END
