#pragma once

#include <userver/proto-structs/io/fwd.hpp>
#include <userver/proto-structs/io/std/int32_t.hpp>
#include <userver/proto-structs/type_mapping.hpp>

namespace ups = USERVER_NAMESPACE::proto_structs;

namespace messages {
class Empty;
class Simple;
}  // namespace messages

namespace structs {

struct Empty {
    using ProtobufMessage = messages::Empty;
};

struct Simple {
    using ProtobufMessage = int;  // non-sense
    int32_t f1 = {};
};

Empty ReadProtoStruct(ups::io::ReadContext&, ups::io::To<Empty>, const messages::Empty&);
void WriteProtoStruct(ups::io::WriteContext&, const Empty&, messages::Empty&);

Simple ReadProtoStruct(ups::io::ReadContext&, ups::io::To<Simple>, const messages::Simple&);
void WriteProtoStruct(ups::io::WriteContext&, const Simple&, messages::Simple&);

void CheckSimpleEqual(const Simple&, const messages::Simple&);

}  // namespace structs

namespace proto_structs::traits {

template <>
struct CompatibleMessageTrait<::structs::Simple> {
    using Type = ::messages::Simple;
};

}  // namespace proto_structs::traits
