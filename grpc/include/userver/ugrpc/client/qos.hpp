#pragma once

#include <chrono>
#include <optional>
#include <string>
#include <unordered_map>

#include <grpcpp/client_context.h>

#include <userver/formats/json_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace testsuite {
class GrpcControl;
}

namespace ugrpc::client {

struct Qos final {
  std::optional<std::chrono::milliseconds> timeout;
};

Qos Parse(const formats::json::Value& value, formats::parse::To<Qos>);

void ApplyQos(grpc::ClientContext& context, const Qos& qos,
              const testsuite::GrpcControl& testsuite_control);

}  // namespace ugrpc::client

USERVER_NAMESPACE_END
