#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

#include <google/protobuf/arena.h>
#include <gtest/gtest.h>

#include <userver/grpc-proto-structs/server/impl/method_result_helpers.hpp>
#include <userver/grpc-proto-structs/server/stream.hpp>

#include <tests/messages.pb.h>

USERVER_NAMESPACE_BEGIN

namespace {

struct ArenaStruct {
    using ProtobufMessage = sample::ugrpc::GreetingResponse;
    std::string name;
};

ArenaStruct ReadProtoStruct(
    proto_structs::io::ReadContext&,
    proto_structs::io::To<ArenaStruct>,
    const sample::ugrpc::GreetingResponse& message
) {
    return {message.name()};
}

void WriteProtoStruct(
    proto_structs::io::WriteContext&,
    const ArenaStruct& value,
    sample::ugrpc::GreetingResponse& message
) {
    message.set_name(value.name);
}

struct ArenaOperationCleanup {
    int& destroyed;
    ~ArenaOperationCleanup() { ++destroyed; }
};

class InspectingMessageStream final
    : public ugrpc::server::ReaderWriter<sample::ugrpc::GreetingResponse, sample::ugrpc::GreetingResponse> {
public:
    explicit InspectingMessageStream(int& destroyed)
        : destroyed_(destroyed)
    {}

    bool Read(sample::ugrpc::GreetingResponse& message) override {
        CheckMessage(message);
        if (throw_on_read_) {
            throw std::runtime_error("read failed");
        }
        if (!std::exchange(has_request_, false)) {
            message.set_name("ignored");
            return false;
        }
        message.set_name("value");
        return true;
    }

    void Write(sample::ugrpc::GreetingResponse&& message) override {
        ++without_options_;
        CheckMessage(message);
        EXPECT_EQ(message.name(), "value");
    }

    void Write(sample::ugrpc::GreetingResponse&& message, const grpc::WriteOptions&) override {
        ++with_options_;
        CheckMessage(message);
        EXPECT_EQ(message.name(), "value");
    }

    void ThrowOnRead() { throw_on_read_ = true; }

    int GetWritesWithoutOptions() const { return without_options_; }
    int GetWritesWithOptions() const { return with_options_; }

private:
    void CheckMessage(sample::ugrpc::GreetingResponse& message) {
        ASSERT_NE(message.GetArena(), nullptr);
        if (message.GetArena()) {
            message.GetArena()->Own(new ArenaOperationCleanup{destroyed_});
        }
    }

    int& destroyed_;
    bool has_request_{true};
    bool throw_on_read_{false};
    int without_options_{0};
    int with_options_{0};
};

}  // namespace

TEST(GrpcProtoStructArena, ArenaResultConversion) {
    google::protobuf::Arena arena;
    ugrpc::server::Result<ArenaStruct> result(ArenaStruct{"response"});
    auto converted = grpc_proto_structs::server::impl::ConvertMethodResult(std::move(result), arena);
    EXPECT_TRUE(converted.IsSuccess());
    EXPECT_EQ(converted.GetResponse().GetArena(), &arena);
    EXPECT_EQ(converted.GetResponse().name(), "response");
    ugrpc::server::StreamingResult<ArenaStruct> streaming(ArenaStruct{"last"});
    auto converted_streaming = grpc_proto_structs::server::impl::ConvertMethodResult(std::move(streaming), arena);
    EXPECT_TRUE(converted_streaming.HasLastResponse());
    EXPECT_EQ(converted_streaming.GetLastResponse().GetArena(), &arena);
    EXPECT_EQ(converted_streaming.GetLastResponse().name(), "last");
}

TEST(GrpcProtoStructArena, HeapResultConversion) {
    auto converted = grpc_proto_structs::server::impl::ConvertMethodResult(
        ugrpc::server::Result<ArenaStruct>(ArenaStruct{}),
        std::nullopt
    );
    EXPECT_EQ(converted.GetResponse().GetArena(), nullptr);
    auto converted_streaming = grpc_proto_structs::server::impl::ConvertMethodResult(
        ugrpc::server::StreamingResult<ArenaStruct>(ArenaStruct{"heap"}),
        std::nullopt
    );
    EXPECT_EQ(converted_streaming.GetLastResponse().GetArena(), nullptr);
    EXPECT_EQ(converted_streaming.GetLastResponse().name(), "heap");
    auto error = grpc_proto_structs::server::impl::ConvertMethodResult(
        ugrpc::server::Result<ArenaStruct>(grpc::Status{grpc::StatusCode::INTERNAL, "failure"}),
        std::nullopt
    );
    EXPECT_FALSE(error.IsSuccess());
    auto empty = grpc_proto_structs::server::impl::ConvertMethodResult(
        ugrpc::server::StreamingResult<ArenaStruct>(grpc::Status::OK),
        std::nullopt
    );
    EXPECT_FALSE(empty.HasLastResponse());
}

TEST(GrpcProtoStructArena, StreamingReadExceptionReleasesArena) {
    int destroyed = 0;
    InspectingMessageStream raw(destroyed);
    raw.ThrowOnRead();
    grpc_proto_structs::server::Reader<ArenaStruct> stream(raw);
    ArenaStruct request{"original"};
    EXPECT_THROW(stream.Read(request), std::runtime_error);
    EXPECT_EQ(request.name, "original");
    EXPECT_EQ(destroyed, 1);
}

TEST(GrpcProtoStructArena, StreamingUsesLocalArena) {
    int destroyed = 0;
    InspectingMessageStream raw(destroyed);
    grpc_proto_structs::server::ReaderWriter<ArenaStruct, ArenaStruct> stream(raw);
    ArenaStruct request;
    EXPECT_TRUE(stream.Read(request));
    EXPECT_EQ(destroyed, 1);
    EXPECT_EQ(request.name, "value");
    EXPECT_FALSE(stream.Read(request));
    EXPECT_EQ(destroyed, 2);
    EXPECT_EQ(request.name, "value");
    stream.Write(std::move(request));
    stream.Write(ArenaStruct{"value"}, {});
    stream.WriteCopy(ArenaStruct{"value"}, grpc::WriteOptions{});
    stream.WriteCopy(ArenaStruct{"value"});
    EXPECT_EQ(raw.GetWritesWithoutOptions(), 0);
    EXPECT_EQ(raw.GetWritesWithOptions(), 4);
    EXPECT_EQ(destroyed, 6);
}

USERVER_NAMESPACE_END
