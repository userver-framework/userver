#pragma once

#include <fmt/format.h>
#include <string_view>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql {

void AssertWhenImplemented(bool is_implemented, std::string_view what);

}  // namespace storages::mysql

USERVER_NAMESPACE_END
