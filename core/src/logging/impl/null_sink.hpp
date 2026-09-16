#pragma once

#include <logging/impl/base_sink.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging::impl {

class NullSink final : public BaseSink {
public:
    NullSink() = default;

    void Write(std::span<const struct iovec> /*log*/) override {}
};

}  // namespace logging::impl

USERVER_NAMESPACE_END
