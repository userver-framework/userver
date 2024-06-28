#include <userver/chaotic/io/boost/uuids/uuid.hpp>

#include <userver/utils/boost_uuid4.hpp>

USERVER_NAMESPACE_BEGIN

namespace chaotic::convert {

boost::uuids::uuid Convert(
    std::string&& str,
    USERVER_NAMESPACE::chaotic::convert::To<boost::uuids::uuid>) {
  return USERVER_NAMESPACE::utils::BoostUuidFromString(str);
}

std::string Convert(const boost::uuids::uuid& uuid,
                    USERVER_NAMESPACE::chaotic::convert::To<std::string>) {
  return USERVER_NAMESPACE::utils::ToString(uuid);
}

}  // namespace chaotic::convert

USERVER_NAMESPACE_END
