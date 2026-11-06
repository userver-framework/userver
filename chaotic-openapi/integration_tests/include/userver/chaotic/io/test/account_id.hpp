#pragma once

#include <boost/uuid/uuid.hpp>

#include <userver/utils/strong_typedef.hpp>

namespace test {

using AccountId = USERVER_NAMESPACE::utils::StrongTypedef<class AccountIdTag, boost::uuids::uuid>;

}  // namespace test
