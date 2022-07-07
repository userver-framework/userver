#pragma once

#include <userver/utils/strong_typedef.hpp>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq {

using Queue = utils::StrongTypedef<class QueueTag, std::string>;
using Exchange = utils::StrongTypedef<class ExchangeTag, std::string>;

}

USERVER_NAMESPACE_END
