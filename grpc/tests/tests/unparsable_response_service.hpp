#pragma once

#include <atomic>
#include <cstddef>
#include <string_view>

#include <grpcpp/support/byte_buffer.h>
#include <grpcpp/support/slice.h>
#include <grpcpp/support/status.h>
#include <gtest/gtest.h>

#include <userver/ugrpc/server/generic_service_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace tests {

inline constexpr std::string_view kUnparsableResponseBytes = "\x0a\x7f";
inline constexpr std::size_t kUnparsableResponseSize = kUnparsableResponseBytes.size();

inline grpc::ByteBuffer MakeUnparsableResponseBytes() {
    const grpc::Slice slice{kUnparsableResponseBytes.data(), kUnparsableResponseBytes.size()};
    return grpc::ByteBuffer{&slice, 1};
}

class UnparsableResponseGenericService final : public ugrpc::server::GenericServiceBase {
public:
    GenericResult Handle(GenericCallContext&, GenericReaderWriter& stream) override {
        grpc::ByteBuffer request_bytes;
        EXPECT_TRUE(stream.Read(request_bytes));
        return MakeUnparsableResponseBytes();
    }
};

class UnparsableThenTrailersOnlyGenericService final : public ugrpc::server::GenericServiceBase {
public:
    GenericResult Handle(GenericCallContext&, GenericReaderWriter& stream) override {
        grpc::ByteBuffer request_bytes;
        EXPECT_TRUE(stream.Read(request_bytes));
        if (calls_++ == 0) {
            return MakeUnparsableResponseBytes();
        }
        return grpc::Status::OK;
    }

private:
    std::atomic<std::size_t> calls_{0};
};

}  // namespace tests

USERVER_NAMESPACE_END
