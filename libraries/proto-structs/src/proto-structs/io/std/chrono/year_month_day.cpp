#include <userver/proto-structs/io/std/chrono/year_month_day.hpp>

#include <cstdint>

#include <fmt/format.h>
#include <google/type/date.pb.h>

#include <userver/proto-structs/io/context.hpp>

namespace proto_structs::io {

constexpr int kMaxYearInGoogleDate = 9999;
constexpr int kMinYearInGoogleDate = 1;

std::chrono::year_month_day
ReadProtoStruct(ReadContext& ctx, To<std::chrono::year_month_day>, const ::google::type::Date& msg) {
    std::chrono::year_month_day result = {};

    if (msg.year() < kMinYearInGoogleDate || msg.year() > kMaxYearInGoogleDate) {
        ctx.AddError(fmt::format(
            "'year' component value {} is not in a valid range [{}, {}]",
            msg.year(),
            kMinYearInGoogleDate,
            kMaxYearInGoogleDate
        ));
        return result;
    }

    result = {
        std::chrono::year{msg.year()},
        std::chrono::month{static_cast<unsigned>(msg.month())},
        std::chrono::day{static_cast<unsigned>(msg.day())}};

    if (!result.ok()) {
        ctx.AddError(fmt::format("{}/{}/{} (y/m/d) does not represent a valid date", msg.year(), msg.month(), msg.day())
        );
        return result;
    }

    return result;
}

void WriteProtoStruct(WriteContext& ctx, const std::chrono::year_month_day& obj, ::google::type::Date& msg) {
    if (!obj.ok()) {
        ctx.AddError(fmt::format(
            "{}/{}/{} (y/m/d) does not represent a valid date",
            static_cast<int>(obj.year()),
            static_cast<unsigned>(obj.month()),
            static_cast<unsigned>(obj.day())
        ));
        return;
    }

    if (static_cast<int>(obj.year()) < kMinYearInGoogleDate || static_cast<int>(obj.year()) > kMaxYearInGoogleDate) {
        ctx.AddError(fmt::format(
            "'year' component value {} is not in a valid range [{}, {}]",
            static_cast<int>(obj.year()),
            kMinYearInGoogleDate,
            kMaxYearInGoogleDate
        ));
        return;
    }

    msg.set_year(static_cast<int32_t>(obj.year()));
    msg.set_month(static_cast<int32_t>(static_cast<unsigned>(obj.month())));
    msg.set_day(static_cast<int32_t>(static_cast<unsigned>(obj.day())));
}

}  // namespace proto_structs::io
