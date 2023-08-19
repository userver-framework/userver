#pragma once

#include <array>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

#include <userver/formats/json/value.hpp>
#include <userver/formats/json/value_builder.hpp>

#include "digest_context.hpp"

USERVER_NAMESPACE_BEGIN

namespace server::handlers::auth {

class DigestParsing {
  formats::json::ValueBuilder directive_mapping;

 public:
  void ParseAuthInfo(std::string_view header_value);
  DigestContextFromClient GetClientContext();
};

}  // namespace server::handlers::auth

USERVER_NAMESPACE_END