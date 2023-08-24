#include <userver/formats/serialize/boost_uuid.hpp>

#include <boost/uuid/uuid_io.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::serialize::detail {

std::string ToString(const boost::uuids::uuid& value) {
  return boost::uuids::to_string(value);
}

}  // namespace formats::serialize::detail

USERVER_NAMESPACE_END
