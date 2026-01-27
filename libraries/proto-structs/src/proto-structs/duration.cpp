#include <userver/proto-structs/duration.hpp>

#include <fmt/format.h>

#include <userver/proto-structs/exceptions.hpp>
#include <userver/utils/impl/internal_tag.hpp>

USERVER_NAMESPACE_BEGIN

namespace proto_structs {

Duration::Duration(utils::impl::InternalTag, std::int64_t seconds, std::int32_t nanos)
    : Duration(std::chrono::seconds{seconds}, std::chrono::nanoseconds{nanos})
{}

void Duration::ThrowError(const std::chrono::seconds& seconds, const std::chrono::nanoseconds& nanos) {
    throw ValueError(fmt::format("Duration '{}s.{}ns' is invalid or out of range", seconds.count(), nanos.count()));
}

void Duration::ThrowError(const char* reason) { throw ValueError(reason); }

}  // namespace proto_structs

USERVER_NAMESPACE_END
