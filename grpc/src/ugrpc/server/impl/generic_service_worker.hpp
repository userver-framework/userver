#pragma once

#include <grpcpp/generic/async_generic_service.h>

#include <userver/ugrpc/server/impl/service_worker.hpp>
#include <userver/utils/box.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server {
class GenericServiceBase;
}  // namespace ugrpc::server

namespace ugrpc::server::impl {

class GenericServiceWorker final {
public:
    GenericServiceWorker(GenericServiceBase& service, ServiceSettings&& settings);

    GenericServiceWorker(GenericServiceWorker&&) noexcept;
    GenericServiceWorker& operator=(GenericServiceWorker&&) noexcept;
    ~GenericServiceWorker();

    grpc::AsyncGenericService& GetService();

    void Start();

private:
    struct Impl;
    utils::Box<Impl> impl_;
};

}  // namespace ugrpc::server::impl

USERVER_NAMESPACE_END
