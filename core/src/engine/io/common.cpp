#include <userver/engine/io/common.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::io {

// Forcing vtable instantiation only in this translation unit
ReadableBase::~ReadableBase() = default;
WritableBase::~WritableBase() = default;
RwBase::~RwBase() = default;

}  // namespace engine::io

USERVER_NAMESPACE_END
