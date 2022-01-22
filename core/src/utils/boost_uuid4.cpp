#include <userver/utils/boost_uuid4.hpp>

#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <userver/utils/rand.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::generators {

boost::uuids::uuid GenerateBoostUuid() {
  thread_local boost::uuids::basic_random_generator<RandomBase> generator(
      DefaultRandom());
  return generator();
}

namespace impl {

std::string ToString(const boost::uuids::uuid& uuid) {
  return boost::uuids::to_string(uuid);
}

}  // namespace impl

}  // namespace utils::generators

USERVER_NAMESPACE_END
