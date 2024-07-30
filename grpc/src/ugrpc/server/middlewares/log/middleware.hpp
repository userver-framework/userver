#pragma once

#include <userver/ugrpc/server/middlewares/base.hpp>
#include <userver/ugrpc/server/storage_context.hpp>
#include <userver/utils/any_storage.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::middlewares::log {

inline const utils::AnyStorageDataTag<StorageContext, bool> kIsFirstRequest;

class Middleware final : public MiddlewareBase {
 public:
  struct Settings {
    size_t max_msg_size{};
    USERVER_NAMESPACE::logging::Level msg_log_level{};
    std::optional<USERVER_NAMESPACE::logging::Level> local_log_level;
  };

  explicit Middleware(const Settings& settings);

  void Handle(MiddlewareCallContext& context) const override;

  void CallRequestHook(const MiddlewareCallContext& context,
                       google::protobuf::Message& request) override;

  void CallResponseHook(const MiddlewareCallContext& context,
                        google::protobuf::Message& response) override;

 private:
  Settings settings_;
};

}  // namespace ugrpc::server::middlewares::log

USERVER_NAMESPACE_END
