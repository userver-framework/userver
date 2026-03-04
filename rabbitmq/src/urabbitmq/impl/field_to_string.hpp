#pragma once

#include <string>

#include <amqpcpp.h>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq::impl {

std::string FieldToString(const AMQP::Field& field);

}  // namespace urabbitmq::impl

USERVER_NAMESPACE_END
