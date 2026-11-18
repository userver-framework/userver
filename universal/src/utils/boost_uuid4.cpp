#include <userver/utils/boost_uuid4.hpp>

#include <boost/uuid/string_generator.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <utils/impl/uuid_random.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils {

namespace generators {

boost::uuids::uuid GenerateBoostUuid() {
    boost::uuids::uuid uuid{};
    utils::impl::FillUuidRandomBytes({uuid.begin(), uuid.end()});
    uuid.data[6] = (uuid.data[6] & 0x0F) | 0x40;  // Version 4
    uuid.data[8] = (uuid.data[8] & 0x3F) | 0x80;  // RFC variant
    return uuid;
}

}  // namespace generators

boost::uuids::uuid BoostUuidFromString(std::string_view str) {
    return boost::uuids::string_generator{}(str.begin(), str.end());
}

std::string ToString(const boost::uuids::uuid& uuid) { return boost::uuids::to_string(uuid); }

}  // namespace utils

USERVER_NAMESPACE_END
