#include <userver/utils/boost_uuid4.hpp>

#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/string_generator.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <userver/compiler/thread_local.hpp>
#include <userver/utils/rand.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils {

namespace generators {

boost::uuids::uuid GenerateBoostUuid() {
    auto local_gen = impl::UseLocalRandomImpl();
    boost::uuids::basic_random_generator generator{*local_gen};
    return generator();
}

}  // namespace generators

boost::uuids::uuid BoostUuidFromString(std::string_view str) {
    return boost::uuids::string_generator{}(str.begin(), str.end());
}

std::string ToString(const boost::uuids::uuid& uuid) { return boost::uuids::to_string(uuid); }

}  // namespace utils

USERVER_NAMESPACE_END
