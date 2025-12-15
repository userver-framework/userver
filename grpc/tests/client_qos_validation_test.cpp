#include <gtest/gtest.h>

#include <ugrpc/client/impl/client_qos_validation.hpp>
#include <userver/utils/default_dict.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

class ValidationTest : public ::testing::Test {};

TEST_F(ValidationTest, ValidPaths) {
    EXPECT_EQ(IsValidRpcMethodPath("sample.ugrpc.Service/Method"), std::nullopt);
    EXPECT_EQ(IsValidRpcMethodPath("com.example.Service/Method"), std::nullopt);
    EXPECT_EQ(IsValidRpcMethodPath("a.b.c.Service/Method"), std::nullopt);
    EXPECT_EQ(IsValidRpcMethodPath("test.Service2/Method2"), std::nullopt);
    EXPECT_EQ(IsValidRpcMethodPath(utils::kDefaultDictDefaultName), std::nullopt);
}

TEST_F(ValidationTest, InvalidPaths) {
    EXPECT_EQ(IsValidRpcMethodPath(""), RpcPathValidationError::kEmptyPath);
    EXPECT_EQ(IsValidRpcMethodPath("/Service/Method"), RpcPathValidationError::kLeadingSlash);
    EXPECT_EQ(IsValidRpcMethodPath("Service"), RpcPathValidationError::kMissingSlash);
    EXPECT_EQ(IsValidRpcMethodPath("Service/Method/Extra"), RpcPathValidationError::kTooManySlashes);
    EXPECT_EQ(IsValidRpcMethodPath("Service/"), RpcPathValidationError::kEmptyMethod);
    EXPECT_EQ(IsValidRpcMethodPath("NoDotsInService/Method"), RpcPathValidationError::kMissingDotInService);
    EXPECT_EQ(IsValidRpcMethodPath("Service-Name/Method"), RpcPathValidationError::kMissingDotInService);
    EXPECT_EQ(IsValidRpcMethodPath("1Service/Method"), RpcPathValidationError::kMissingDotInService);
}

TEST_F(ValidationTest, EdgeCases) {
    EXPECT_EQ(IsValidRpcMethodPath("a.b/c"), std::nullopt);
    EXPECT_EQ(IsValidRpcMethodPath("_a._b/_c"), std::nullopt);
    EXPECT_EQ(IsValidRpcMethodPath("/"), RpcPathValidationError::kLeadingSlash);
    EXPECT_EQ(IsValidRpcMethodPath("a/b"), RpcPathValidationError::kMissingDotInService);
}

// Tests for IsMethodForService function
TEST_F(ValidationTest, ServiceMatches) {
    EXPECT_TRUE(IsMethodOfService("sample.ugrpc.Service/Method", "sample.ugrpc.Service"));
    EXPECT_TRUE(IsMethodOfService("com.example.Service/Method", "com.example.Service"));
    EXPECT_TRUE(IsMethodOfService("a.b.c/Method", "a.b.c"));
}

TEST_F(ValidationTest, ServiceNoMatches) {
    EXPECT_FALSE(IsMethodOfService("sample.ugrpc.Service/Method", "different.Service"));
    EXPECT_FALSE(IsMethodOfService("com.example.Service/Method", "com.example.Other"));
    EXPECT_FALSE(IsMethodOfService("sample.ugrpc.Service/Method", "sample.ugrpc"));
    EXPECT_FALSE(IsMethodOfService(utils::kDefaultDictDefaultName, "any.service"));
}

TEST_F(ValidationTest, ValidationFlow) {
    std::string method = "sample.ugrpc.Service/Method";
    std::string service = "sample.ugrpc.Service";

    EXPECT_EQ(IsValidRpcMethodPath(method), std::nullopt);
    EXPECT_TRUE(IsMethodOfService(method, service));

    std::string invalid = "/InvalidPath";
    EXPECT_EQ(IsValidRpcMethodPath(invalid), RpcPathValidationError::kLeadingSlash);

    std::string other = "other.service.Name/Method";
    EXPECT_EQ(IsValidRpcMethodPath(other), std::nullopt);
    EXPECT_FALSE(IsMethodOfService(other, service));

    EXPECT_EQ(IsValidRpcMethodPath(utils::kDefaultDictDefaultName), std::nullopt);
    EXPECT_FALSE(IsMethodOfService(utils::kDefaultDictDefaultName, service));
}

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
