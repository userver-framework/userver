#pragma once

#include <gmock/gmock-function-mocker.h>

#include <tests/unit_test_service.usrv.pb.hpp>

USERVER_NAMESPACE_BEGIN

namespace tests {

class UnitTestServiceGmock : public sample::ugrpc::UnitTestServiceBase {
public:
    MOCK_METHOD(SayHelloResult, SayHello, (CallContext&, sample::ugrpc::GreetingRequest&&), (override));

    MOCK_METHOD(
        ReadManyResult,
        ReadMany,
        (CallContext&, sample::ugrpc::StreamGreetingRequest&&, ReadManyWriter&),
        (override)
    );

    MOCK_METHOD(WriteManyResult, WriteMany, (CallContext&, WriteManyReader&), (override));

    MOCK_METHOD(ChatResult, Chat, (CallContext&, ChatReaderWriter&), (override));
};

}  // namespace tests

USERVER_NAMESPACE_END
