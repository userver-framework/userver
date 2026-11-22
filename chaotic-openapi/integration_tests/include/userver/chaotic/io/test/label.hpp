#pragma once

#include <string>

#include <userver/utils/strong_typedef.hpp>

namespace test {

using Label = USERVER_NAMESPACE::utils::StrongTypedef<class LabelTag, std::string>;

}  // namespace test
