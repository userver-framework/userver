#include <userver/proto-structs/io/userver/proto_structs/date_conv.hpp>

#include <google/type/date.pb.h>

#include <userver/proto-structs/exceptions.hpp>
#include <userver/proto-structs/io/context.hpp>
#include <userver/utils/impl/internal_tag.hpp>

USERVER_NAMESPACE_BEGIN

namespace proto_structs::io {

Date ReadProtoStruct(ReadContext& ctx, To<Date>, const ::google::type::Date& msg) {
    try {
        return Date(utils::impl::InternalTag{}, msg.year(), msg.month(), msg.day());
    } catch (const ValueError& e) {
        ctx.AddError(e.what());
        return Date{};
    }
}

void WriteProtoStruct(WriteContext&, const Date& obj, ::google::type::Date& msg) {
    msg.set_year(obj.YearNum());
    msg.set_month(obj.MonthNum());
    msg.set_day(obj.DayNum());
}

}  // namespace proto_structs::io

USERVER_NAMESPACE_END
