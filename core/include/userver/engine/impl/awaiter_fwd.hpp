#pragma once

#include <memory>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

class Awaiter;
struct AwaiterDeleter;

using AwaiterPtr = std::unique_ptr<Awaiter, AwaiterDeleter>;

}  // namespace engine::impl

USERVER_NAMESPACE_END
