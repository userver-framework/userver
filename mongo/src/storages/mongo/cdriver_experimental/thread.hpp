#pragma once

#include <bson/bson.h>

USERVER_NAMESPACE_BEGIN

namespace storages::mongo::impl::cdriver_experimental {

const bson_thread_backend_t& GetThreadBackend() noexcept;

class MongocGlobalLifecycleScope final {
public:
    MongocGlobalLifecycleScope();
    ~MongocGlobalLifecycleScope();

    MongocGlobalLifecycleScope(const MongocGlobalLifecycleScope&) = delete;
    MongocGlobalLifecycleScope& operator=(const MongocGlobalLifecycleScope&) = delete;
};

}  // namespace storages::mongo::impl::cdriver_experimental

USERVER_NAMESPACE_END
