#pragma once

#include <cstdint>
#include <string_view>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres::utils {

std::size_t StrHash(std::string_view str) noexcept;

struct StringViewHash {
    std::size_t operator()(std::string_view str) const noexcept { return StrHash(str); }
};

}  // namespace storages::postgres::utils

USERVER_NAMESPACE_END
