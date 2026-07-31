#include <userver/storages/postgres/detail/string_hash.hpp>

#include <boost/functional/hash.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres::utils {

std::size_t StrHash(std::string_view str) noexcept {
    auto seed = str.size();
    boost::hash_range(seed, str.data(), str.data() + str.size());
    return seed;
}

}  // namespace storages::postgres::utils

USERVER_NAMESPACE_END
