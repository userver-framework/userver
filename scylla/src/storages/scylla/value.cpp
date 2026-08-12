#include <userver/storages/scylla/value.hpp>

#include <array>
#include <cstring>
#include <mutex>

#include <cassandra.h>

#include <userver/storages/scylla/exception.hpp>
#include <userver/utils/algo.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::scylla {

namespace {

CassUuidGen* GlobalUuidGen() {
    static CassUuidGen* gen = cass_uuid_gen_new();
    return gen;
}

CassUuid ToCass(const Uuid& u) {
    CassUuid out;
    out.time_and_version = u.time_and_version;
    out.clock_seq_and_node = u.clock_seq_and_node;
    return out;
}

Uuid FromCass(const CassUuid& cu) { return Uuid{cu.time_and_version, cu.clock_seq_and_node}; }

}  // namespace

Uuid Uuid::Random() {
    CassUuid out;
    cass_uuid_gen_random(GlobalUuidGen(), &out);
    return FromCass(out);
}

Uuid Uuid::TimeBased() {
    CassUuid out;
    cass_uuid_gen_time(GlobalUuidGen(), &out);
    return FromCass(out);
}

Uuid Uuid::FromString(std::string_view text) {
    const std::string zero_terminated{text};
    CassUuid out;
    if (cass_uuid_from_string(zero_terminated.c_str(), &out) != CASS_OK) {
        throw InvalidQueryArgumentException(utils::StrCat("Uuid::FromString: invalid UUID '", text, "'"));
    }
    return FromCass(out);
}

std::string Uuid::ToString() const {
    std::array<char, CASS_UUID_STRING_LENGTH> buf{};
    cass_uuid_string(ToCass(*this), buf.data());
    return std::string{buf.data()};
}

}  // namespace storages::scylla

USERVER_NAMESPACE_END
