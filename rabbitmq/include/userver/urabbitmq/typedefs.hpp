#pragma once

#include <userver/utils/strong_typedef.hpp>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq {

/// StrongTypedef alias for a queue name.
using Queue = utils::StrongTypedef<class QueueTag, std::string>;

/// StrongTypedef alias for an exchange name.
using Exchange = utils::StrongTypedef<class ExchangeTag, std::string>;

}  // namespace urabbitmq

USERVER_NAMESPACE_END
