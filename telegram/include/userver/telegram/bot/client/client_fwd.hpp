#pragma once

#include <memory>

USERVER_NAMESPACE_BEGIN

namespace telegram::bot {

class Client;
using ClientPtr = std::shared_ptr<Client>;

}  // namespace telegram::bot

USERVER_NAMESPACE_END
