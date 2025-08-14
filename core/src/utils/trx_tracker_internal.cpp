#include "trx_tracker_internal.hpp"

USERVER_NAMESPACE_BEGIN

namespace utils::trx_tracker {

namespace {

bool trx_tracker_enabled = false;

}  // namespace

GlobalEnabler::GlobalEnabler(bool enable) { trx_tracker_enabled = enable; }

GlobalEnabler::~GlobalEnabler() { trx_tracker_enabled = false; }

bool IsEnabled() noexcept { return trx_tracker_enabled; }

}  // namespace utils::trx_tracker

USERVER_NAMESPACE_END
