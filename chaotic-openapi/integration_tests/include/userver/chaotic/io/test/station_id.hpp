#pragma once

#include <boost/uuid/uuid.hpp>

#include <userver/utils/strong_typedef.hpp>

namespace test {

using StationId = USERVER_NAMESPACE::utils::StrongTypedef<class StationIdTag, boost::uuids::uuid>;

}  // namespace test
