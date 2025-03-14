#pragma once

#include <memory>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::impl {

class Statement;

using InputBindingsPimpl = std::shared_ptr<Statement>;

using InputBindingsFwd = Statement;

}  // namespace storages::sqlite::impl

USERVER_NAMESPACE_END
