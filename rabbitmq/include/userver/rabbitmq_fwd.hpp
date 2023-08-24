#pragma once

/// @file userver/rabbitmq_fwd.hpp
/// This file is for forward-declaring some headers that are required for
/// working with RabbitMQ userver component.

#include <memory>

USERVER_NAMESPACE_BEGIN

namespace components {
class RabbitMQ;
}

namespace urabbitmq {

class Client;
using ClientPtr = std::shared_ptr<Client>;

class AdminChannel;
class Channel;
class ReliableChannel;

class Queue;
class Exchange;

}  // namespace urabbitmq

USERVER_NAMESPACE_END
