#include <userver/proto-structs/io/std/chrono/year_month_day_conv.hpp>

#include <google/type/date.pb.h>

#include <userver/proto-structs/date.hpp>
#include <userver/proto-structs/exceptions.hpp>
#include <userver/proto-structs/io/context.hpp>
#include <userver/utils/impl/internal_tag.hpp>

USERVER_NAMESPACE_BEGIN

namespace proto_structs::io {

std::chrono::year_month_day ReadProtoStruct(
    ReadContext& ctx,
    To<std::chrono::year_month_day>,
    const ::google::type::Date& msg
) {
    try {
        Date date(utils::impl::InternalTag{}, msg.year(), msg.month(), msg.day());

        if (!date.HasYearMonthDay()) {
            ctx.AddError("full date is expected for 'std::chrono::year_month_day' proto struct field");
            return std::chrono::year_month_day{std::chrono::year{0}, std::chrono::month{0}, std::chrono::day{0}};
        }

        return date.ToChronoDate();
    } catch (const ValueError& e) {
        ctx.AddError(e.what());
        return std::chrono::year_month_day{std::chrono::year{0}, std::chrono::month{0}, std::chrono::day{0}};
    }
}

void WriteProtoStruct(WriteContext& ctx, const std::chrono::year_month_day& obj, ::google::type::Date& msg) {
    try {
        Date date{obj};
        msg.set_year(date.YearNum());
        msg.set_month(date.MonthNum());
        msg.set_day(date.DayNum());
    } catch (const ValueError& e) {
        ctx.AddError(e.what());
    }
}

}  // namespace proto_structs::io

USERVER_NAMESPACE_END
