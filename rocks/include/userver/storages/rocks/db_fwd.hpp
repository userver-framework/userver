#pragma once

#include <memory>

USERVER_NAMESPACE_BEGIN

namespace storages::rocks {
class Db;
using DbPtr = std::shared_ptr<Db>;
}  // namespace storages::rocks

USERVER_NAMESPACE_END
