#pragma once

#include <userver/utils/datetime.hpp>

namespace storages {
namespace postgres {
namespace detail {

using SteadyClock = ::utils::datetime::SteadyClock;

}  // namespace detail
}  // namespace postgres
}  // namespace storages
