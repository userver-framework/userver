#pragma once

#include <memory>

namespace engine {
namespace io {
namespace impl {

class FdControl;

using FdControlHolder = std::shared_ptr<FdControl>;

}  // namespace impl
}  // namespace io
}  // namespace engine
