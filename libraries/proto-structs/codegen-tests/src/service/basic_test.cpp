#include <gtest/gtest.h>

#include <service/base.structs.usrv.pb.hpp>

namespace ss = service::structs;

USERVER_NAMESPACE_BEGIN

TEST(MyService, Basic) {
    static_assert(std::is_aggregate_v<ss::SomeMessage>);

    // We don't include 'simple/subdirectory/separate_enum.structs.usrv.pb.hpp' here,
    // but that include must be included in 'service/base.structs.usrv.pb.hpp' from method of service::MyService.
    static_assert(std::is_aggregate_v<simple::subdirectory::structs::SeparateEnumMessage>);
    // static_assert(std::is_aggregate_v<google::protobuf::structs::Empty>);
}

USERVER_NAMESPACE_END
