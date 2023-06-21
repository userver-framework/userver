#include <userver/ugrpc/client/qos.hpp>

#include <fmt/chrono.h>
#include <fmt/format.h>
#include <grpcpp/client_context.h>

#include <userver/formats/json/value.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/testsuite/grpc_control.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client {

Qos Parse(const formats::json::Value& value, formats::parse::To<Qos>) {
  auto ms =
      value["timeout-ms"].As<std::optional<std::chrono::milliseconds::rep>>();
  if (!ms) return Qos{};

  return Qos{std::optional<std::chrono::milliseconds>{*ms}};
}

void ApplyQos(grpc::ClientContext& context, const Qos& qos,
              const testsuite::GrpcControl& testsuite_control) {
  if (!qos.timeout) return;

  auto deadline = context.deadline();
  auto now = std::chrono::system_clock::now();
  if (now + std::chrono::hours(100) < deadline) {
    context.set_deadline(now + testsuite_control.MakeTimeout(*qos.timeout));
  }
}

}  // namespace ugrpc::client

USERVER_NAMESPACE_END
