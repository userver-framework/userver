#include <userver/formats/parse/boost_uuid.hpp>

#include <algorithm>
#include <array>
#include <string_view>

#include <userver/utils/boost_uuid4.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::parse::detail {

boost::uuids::uuid ParseString(std::string const& str) {
  return utils::BoostUuidFromString(str);
}

}  // namespace formats::parse::detail

USERVER_NAMESPACE_END
