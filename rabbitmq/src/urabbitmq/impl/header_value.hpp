#pragma once

#include <string>
#include <unordered_map>

#include <userver/urabbitmq/typedefs.hpp>

#include <amqpcpp.h>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq::impl {

HeaderValue ToHeaderValue(const AMQP::Field& field);

std::unordered_map<std::string, HeaderValue> TableToHeaders(const AMQP::Table& table);

void AddHeadersToTable(AMQP::Table& table, const std::unordered_map<std::string, HeaderValue>& headers);

}  // namespace urabbitmq::impl

USERVER_NAMESPACE_END
