#include "struct_simple.hpp"

#include <gtest/gtest.h>

#include <userver/proto-structs/io/impl/read.hpp>
#include <userver/proto-structs/io/impl/write.hpp>
#include <userver/proto-structs/io/std/int32_t_conv.hpp>

#include "messages.pb.h"

namespace structs {

Empty ReadProtoStruct(ups::io::ReadContext&, ups::io::To<Empty>, const messages::Empty&) { return {}; }

void WriteProtoStruct(ups::io::WriteContext&, const Empty&, messages::Empty&) {}

Simple ReadProtoStruct(ups::io::ReadContext& ctx, ups::io::To<Simple>, const messages::Simple& msg) {
    return {
        .f1 = ups::io::impl::ReadField<std::int32_t>(
            ctx,
            ups::io::impl::CreateFieldGetter(msg, messages::Simple::kF1FieldNumber, &messages::Simple::f1)
        )
    };
}

void WriteProtoStruct(ups::io::WriteContext& ctx, const Simple& obj, messages::Simple& msg) {
    ups::io::impl::WriteField(
        ctx,
        obj.f1,
        ups::io::impl::CreateFieldSetter(
            msg,
            messages::Simple::kF1FieldNumber,
            &messages::Simple::set_f1,
            &messages::Simple::clear_f1
        )
    );
}

void CheckSimpleEqual(const Simple& obj, const messages::Simple& msg) { EXPECT_EQ(obj.f1, msg.f1()); }

}  // namespace structs
