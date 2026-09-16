#include "base_sink.hpp"

USERVER_NAMESPACE_BEGIN

namespace logging::impl {

BaseSink::BaseSink() = default;

BaseSink::~BaseSink() = default;

void BaseSink::Reopen(ReopenMode) {}

void BaseSink::SetLevel(Level log_level) { level_.store(log_level); }

Level BaseSink::GetLevel() const { return level_.load(); }

}  // namespace logging::impl

USERVER_NAMESPACE_END
