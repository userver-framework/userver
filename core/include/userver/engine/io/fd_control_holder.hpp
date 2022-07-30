#pragma once

#include <memory>

USERVER_NAMESPACE_BEGIN

namespace engine::io::impl {

class FdControl;

struct FdControlDeleter {
  void operator()(FdControl* ptr) const noexcept;
};

using FdControlHolder = std::unique_ptr<FdControl, FdControlDeleter>;

}  // namespace engine::io::impl

USERVER_NAMESPACE_END
