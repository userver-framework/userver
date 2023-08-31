#pragma once

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

enum class DebugInfoAction {
  kLeaveAsIs,
  kLockInMemory,
};

void InitPhdrCache();

void MLockDebugInfo(DebugInfoAction debug_info);

void TeardownPhdrCacheAndEnableDynamicLoading();

}  // namespace engine::impl

USERVER_NAMESPACE_END
