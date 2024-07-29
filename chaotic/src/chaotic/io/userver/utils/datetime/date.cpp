#include <userver/chaotic/io/userver/utils/datetime/date.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::datetime {

Date Convert(const std::string& value, chaotic::convert::To<Date>) {
  return DateFromRFC3339String(value);
}

std::string Convert(const Date& value, chaotic::convert::To<std::string>) {
  return ToString(value);
}

}  // namespace utils::datetime

USERVER_NAMESPACE_END
