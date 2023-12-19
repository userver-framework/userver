#include <userver/ugrpc/client/qos.hpp>

#include <grpcpp/client_context.h>

#include <userver/formats/json/serialize_duration.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/formats/serialize/common_containers.hpp>
#include <userver/testsuite/grpc_control.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client {

Qos Parse(const formats::json::Value& value, formats::parse::To<Qos>) {
  Qos result;
  const auto ms =
      value["timeout-ms"].As<std::optional<std::chrono::milliseconds::rep>>();
  if (ms) {
    result.timeout = std::chrono::milliseconds{*ms};
  }
  return result;
}

formats::json::Value Serialize(const Qos& qos,
                               formats::serialize::To<formats::json::Value>) {
  formats::json::ValueBuilder result{formats::common::Type::kObject};
  result["timeout-ms"] = qos.timeout;
  return result.ExtractValue();
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
