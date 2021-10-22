#pragma once

#include <memory>

USERVER_NAMESPACE_BEGIN

namespace engine::io::impl {

class FdControl;

using FdControlHolder = std::shared_ptr<FdControl>;

}  // namespace engine::io::impl

USERVER_NAMESPACE_END
