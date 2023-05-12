#pragma once

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

void InitPhdrCacheAndDisableDynamicLoading();

void TeardownPhdrCacheAndEnableDynamicLoading();

}  // namespace engine::impl

USERVER_NAMESPACE_END
