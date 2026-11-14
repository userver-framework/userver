#pragma once

#include <cstdint>

#include <userver/utils/strong_typedef.hpp>

namespace test {

using Count = USERVER_NAMESPACE::utils::StrongTypedef<class CountTag, std::int64_t>;

}  // namespace test
