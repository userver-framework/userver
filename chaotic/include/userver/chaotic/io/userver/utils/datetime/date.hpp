#pragma once

#include <userver/chaotic/convert/to.hpp>
#include <userver/utils/datetime/date.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::datetime {

Date Convert(const std::string& value, chaotic::convert::To<Date>);

std::string Convert(const Date& value, chaotic::convert::To<std::string>);

}  // namespace utils::datetime

USERVER_NAMESPACE_END
