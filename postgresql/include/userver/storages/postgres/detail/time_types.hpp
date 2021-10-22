#pragma once

#include <userver/utils/datetime.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages {
namespace postgres {
namespace detail {

using SteadyClock = USERVER_NAMESPACE::utils::datetime::SteadyClock;

}  // namespace detail
}  // namespace postgres
}  // namespace storages

USERVER_NAMESPACE_END
