#pragma once

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client {

struct Secdist final {
  explicit Secdist(const formats::json::Value& doc)
      : tokens(
            doc["GRPC_TOKENS"].As<std::unordered_map<std::string, std::string>>(
                {})) {}

  std::unordered_map<std::string, std::string> tokens;
};

}  // namespace ugrpc::client

USERVER_NAMESPACE_END
