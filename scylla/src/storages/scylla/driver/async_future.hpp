#pragma once

#include <cassandra.h>

#include <userver/engine/single_use_event.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::scylla::impl::driver {

inline void AsyncWaitFuture(CassFuture* future) noexcept {
    UASSERT(future);
    engine::SingleUseEvent event;
    const auto rc = cass_future_set_callback(
        future,
        [](CassFuture*, void* data) noexcept { static_cast<engine::SingleUseEvent*>(data)->Send(); },
        &event
    );

    UASSERT(rc == CASS_OK);
    (void)rc;
    event.WaitNonCancellable();
}

}  // namespace storages::scylla::impl::driver

USERVER_NAMESPACE_END
