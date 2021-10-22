#include <userver/utils/boost_uuid4.hpp>

#include <boost/random/mersenne_twister.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::generators {

boost::uuids::uuid GenerateBoostUuid() {
  // Can not use std::mt19937_64 because of the different seed() signature
  using RBGenerator = boost::random::mt19937_64;
  thread_local boost::uuids::basic_random_generator<RBGenerator> generator;

  return generator();
}

namespace impl {
std::string ToString(const boost::uuids::uuid& uuid) {
  return boost::uuids::to_string(uuid);
}

}  // namespace impl

}  // namespace utils::generators

USERVER_NAMESPACE_END
