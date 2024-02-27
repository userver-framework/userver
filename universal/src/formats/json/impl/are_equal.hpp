#pragma once

#include <userver/formats/json/impl/types.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::json::impl {

/// Check whether two json trees are equal
bool AreEqual(const Value* lhs, const Value* rhs);

}  // namespace formats::json::impl

USERVER_NAMESPACE_END
