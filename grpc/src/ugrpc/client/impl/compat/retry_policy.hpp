#pragma once

#include <chrono>

#include <userver/formats/json/value.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl::compat {

formats::json::Value ConstructDefaultRetryPolicy();

std::chrono::milliseconds CalculateTotalTimeout(std::chrono::milliseconds timeout, std::uint32_t attempts);

}  // namespace ugrpc::client::impl::compat

USERVER_NAMESPACE_END
