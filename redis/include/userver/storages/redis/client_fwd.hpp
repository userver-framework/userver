#pragma once

#include <memory>

USERVER_NAMESPACE_BEGIN

namespace storages::redis {

class Client;
using ClientPtr = std::shared_ptr<Client>;

class SubscribeClient;
using SubscribeClientPtr = std::shared_ptr<SubscribeClient>;

}  // namespace storages::redis

USERVER_NAMESPACE_END
