#pragma once

#include <cstdint>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

enum class Epoch : std::uint32_t {};

struct NoEpoch {};

}  // namespace engine::impl

USERVER_NAMESPACE_END
