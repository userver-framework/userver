#include <userver/proto-structs/time_of_day.hpp>

#include <fmt/format.h>

#include <userver/proto-structs/exceptions.hpp>
#include <userver/utils/impl/internal_tag.hpp>

USERVER_NAMESPACE_BEGIN

namespace proto_structs {

TimeOfDay::TimeOfDay(
    ::utils::impl::InternalTag,
    std::int32_t hours,
    std::int32_t minutes,
    std::int32_t seconds,
    std::int32_t nanos
)
    : TimeOfDay(
          std::chrono::hours{hours},
          std::chrono::minutes{minutes},
          std::chrono::seconds{seconds},
          std::chrono::nanoseconds{nanos}
      )
{}

void TimeOfDay::ThrowError(
    const std::chrono::hours& hours,
    const std::chrono::minutes& minutes,
    const std::chrono::seconds& seconds,
    const std::chrono::nanoseconds& nanos
) {
    throw ValueError(fmt::format(
        "Time of the day '{}:{}:{}.{}ns' is invalid or out of range",
        hours.count(),
        minutes.count(),
        seconds.count(),
        nanos.count()
    ));
}

void TimeOfDay::ThrowError(const char* reason) { throw ValueError(reason); }

}  // namespace proto_structs

USERVER_NAMESPACE_END
