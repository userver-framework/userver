#pragma once

#include <memory>

namespace engine::io::impl {

class FdControl;

using FdControlHolder = std::shared_ptr<FdControl>;

}  // namespace engine::io::impl
