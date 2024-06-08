#pragma once

#include <memory>

USERVER_NAMESPACE_BEGIN

namespace storages::rocks {

class Client;
using ClientPtr = std::shared_ptr<Client>;

}  // namespace storages::rocks

USERVER_NAMESPACE_END
