#include <userver/utest/utest.hpp>

#include <google/protobuf/util/time_util.h>

#include <userver/formats/json/inline.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/formats/serialize/common_containers.hpp>
#include <userver/logging/log.hpp>

#include <userver/ugrpc/client/client_qos.hpp>
#include <userver/ugrpc/client/impl/compat/channel_arguments_builder.hpp>
#include <userver/ugrpc/impl/to_string.hpp>

#include <tests/unit_test_client.usrv.pb.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

formats::json::Value BuildSimpleRetryPolicyConfig(std::uint32_t max_attempts) {
    formats::json::ValueBuilder retry_policy{formats::common::Type::kObject};
    retry_policy["maxAttempts"] = max_attempts;
    retry_policy["initialBackoff"] = "0.010s";
    retry_policy["maxBackoff"] = "0.300s";
    retry_policy["backoffMultiplier"] = 2;
    retry_policy["retryableStatusCodes"] = formats::json::MakeArray("UNAVAILABLE");
    return retry_policy.ExtractValue();
}

formats::json::Value BuildMethodConfig(
    const std::vector<std::pair<std::optional<std::string_view>, std::optional<std::string_view>>>& name,
    std::optional<google::protobuf::Duration> timeout,
    const formats::json::Value& retry_policy
) {
    formats::json::ValueBuilder method_config{formats::common::Type::kObject};

    for (const auto& [service, method] : name) {
        formats::json::ValueBuilder name_entry{formats::common::Type::kObject};
        if (service.has_value()) {
            name_entry["service"] = service;
        }
        if (method.has_value()) {
            name_entry["method"] = method;
        }
        method_config["name"].PushBack(name_entry.ExtractValue());
    }

    if (timeout.has_value()) {
        method_config["timeout"] = ugrpc::impl::ToString(google::protobuf::util::TimeUtil::ToString(*timeout));
    }

    method_config["retryPolicy"] = retry_policy;

    return method_config.ExtractValue();
}

formats::json::Value
BuildDefaultMethodConfig(std::optional<google::protobuf::Duration> timeout, const formats::json::Value& retry_policy) {
    // If the 'service' field is empty, the 'method' field must be empty, and
    //   this MethodConfig specifies the default for all methods (it's the default
    //   config).
    return BuildMethodConfig({{std::nullopt, std::nullopt}}, timeout, retry_policy);
}

void VerifyMethodConfig(
    const formats::json::Value& method_config,
    std::vector<std::pair<std::optional<std::string_view>, std::optional<std::string_view>>> name,
    std::optional<google::protobuf::Duration> timeout,
    std::optional<std::uint32_t> attempts
) {
    ASSERT_TRUE(method_config["name"].IsArray());
    ASSERT_EQ(name.size(), method_config["name"].GetSize());
    size_t i = 0;
    for (const auto& [service, method] : name) {
        ASSERT_EQ(service, method_config["name"][i]["service"].As<std::optional<std::string>>());
        ASSERT_EQ(method, method_config["name"][i]["method"].As<std::optional<std::string>>());
        ++i;
    }

    ASSERT_EQ(timeout.has_value(), method_config.HasMember("timeout"));
    if (timeout.has_value()) {
        ASSERT_EQ(google::protobuf::util::TimeUtil::ToString(*timeout), method_config["timeout"].As<std::string>());
    }

    ASSERT_EQ(attempts.has_value(), method_config.HasMember("retryPolicy"));
    if (attempts.has_value()) {
        const auto& retry_policy = method_config["retryPolicy"];
        ASSERT_TRUE(retry_policy.IsObject());
        ASSERT_TRUE(retry_policy.HasMember("maxAttempts"));
        ASSERT_EQ(attempts, retry_policy["maxAttempts"].As<std::uint32_t>());
    }
}

}  // namespace

UTEST(ServiceConfigBuilderTest, BuildEmpty) {
    const auto metadata = sample::ugrpc::UnitTestServiceClient::GetMetadata();

    const ugrpc::client::RetryConfig retry_config;

    {
        const ugrpc::client::impl::compat::ServiceConfigBuilder service_config_builder{
            metadata, retry_config, /*static_service_config=*/std::nullopt};
        const auto service_config = service_config_builder.Build(ugrpc::client::ClientQos{});
        ASSERT_TRUE(service_config.IsNull());
    }

    {
        const ugrpc::client::impl::compat::ServiceConfigBuilder service_config_builder{
            metadata, retry_config, /*static_service_config=*/"{}"};
        const auto service_config = service_config_builder.Build(ugrpc::client::ClientQos{});
        ASSERT_TRUE(service_config.IsObject() && service_config.IsEmpty());
    }
}

UTEST(ServiceConfigBuilderTest, Static) {
    const auto metadata = sample::ugrpc::UnitTestServiceClient::GetMetadata();

    const ugrpc::client::RetryConfig retry_config{/*attempts=*/2};

    const ugrpc::client::impl::compat::ServiceConfigBuilder service_config_builder{
        metadata, retry_config, /*static_service_config=*/std::nullopt};
    const auto service_config = service_config_builder.Build(ugrpc::client::ClientQos{});
    LOG_DEBUG() << "service_config: " << service_config;

    ASSERT_TRUE(service_config.HasMember("methodConfig"));
    const auto& method_config = service_config["methodConfig"];
    ASSERT_TRUE(method_config.IsArray());
    ASSERT_EQ(1, method_config.GetSize());

    VerifyMethodConfig(
        method_config[0],
        {{"sample.ugrpc.UnitTestService", "ReadMany"},
         {"sample.ugrpc.UnitTestService", "WriteMany"},
         {"sample.ugrpc.UnitTestService", "Chat"}},
        /*timeout=*/std::nullopt,
        /*attempts=*/retry_config.attempts
    );
}

UTEST(ServiceConfigBuilderTest, Qos) {
    const auto metadata = sample::ugrpc::UnitTestServiceClient::GetMetadata();

    const ugrpc::client::RetryConfig retry_config;

    const auto default_timeout = google::protobuf::util::TimeUtil::MillisecondsToDuration(1000);

    std::uint32_t max_attempts = 5;
    const auto retry_policy_json = BuildSimpleRetryPolicyConfig(max_attempts);
    const auto method_config_json = BuildDefaultMethodConfig(default_timeout, retry_policy_json);
    const auto static_service_config =
        formats::json::MakeObject("methodConfig", formats::json::MakeArray(method_config_json));
    LOG_DEBUG() << "static_service_config: " << static_service_config;

    const ugrpc::client::impl::compat::ServiceConfigBuilder service_config_builder{
        metadata, retry_config, formats::json::ToString(static_service_config)};

    const ugrpc::client::Qos qos_default{/*attempts=*/2, /*timeout=*/std::nullopt};
    ugrpc::client::ClientQos client_qos;
    client_qos.methods.SetDefault(qos_default);

    const auto service_config = service_config_builder.Build(client_qos);
    LOG_DEBUG() << "service_config: " << service_config;

    ASSERT_TRUE(service_config.HasMember("methodConfig"));
    const auto& method_config = service_config["methodConfig"];
    ASSERT_TRUE(method_config.IsArray());
    ASSERT_EQ(2, method_config.GetSize());

    VerifyMethodConfig(
        method_config[0],
        {{"sample.ugrpc.UnitTestService", "ReadMany"},
         {"sample.ugrpc.UnitTestService", "WriteMany"},
         {"sample.ugrpc.UnitTestService", "Chat"}},
        /*timeout=*/default_timeout,
        /*attempts=*/*qos_default.attempts
    );

    VerifyMethodConfig(
        method_config[1],
        {{/*service_name=*/std::nullopt, /*method_name=*/std::nullopt}},
        /*timeout=*/default_timeout,
        /*attempts=*/max_attempts
    );
}

UTEST(ServiceConfigBuilderTest, QosNoRetry) {
    const auto metadata = sample::ugrpc::UnitTestServiceClient::GetMetadata();

    const ugrpc::client::RetryConfig retry_config;

    const auto default_timeout = google::protobuf::util::TimeUtil::MillisecondsToDuration(1000);

    std::uint32_t max_attempts = 5;
    const auto retry_policy_json = BuildSimpleRetryPolicyConfig(max_attempts);
    const auto method_config_json = BuildDefaultMethodConfig(default_timeout, retry_policy_json);
    const auto static_service_config =
        formats::json::MakeObject("methodConfig", formats::json::MakeArray(method_config_json));
    LOG_DEBUG() << "static_service_config: " << static_service_config;

    const ugrpc::client::impl::compat::ServiceConfigBuilder service_config_builder{
        metadata, retry_config, formats::json::ToString(static_service_config)};

    const ugrpc::client::Qos qos_default{/*attempts=*/1, /*timeout=*/std::nullopt};
    ugrpc::client::ClientQos client_qos;
    client_qos.methods.SetDefault(qos_default);

    const auto service_config = service_config_builder.Build(client_qos);
    LOG_DEBUG() << "service_config: " << service_config;

    ASSERT_TRUE(service_config.HasMember("methodConfig"));
    const auto& method_config = service_config["methodConfig"];
    ASSERT_TRUE(method_config.IsArray());
    ASSERT_EQ(2, method_config.GetSize());

    VerifyMethodConfig(
        method_config[0],
        {{"sample.ugrpc.UnitTestService", "ReadMany"},
         {"sample.ugrpc.UnitTestService", "WriteMany"},
         {"sample.ugrpc.UnitTestService", "Chat"}},
        /*timeout=*/default_timeout,
        /*attempts=*/{}
    );

    VerifyMethodConfig(
        method_config[1],
        {{/*service_name=*/std::nullopt, /*method_name=*/std::nullopt}},
        /*timeout=*/default_timeout,
        /*attempts=*/max_attempts
    );
}

UTEST(ServiceConfigBuilderTest, Complex) {
    const auto metadata = sample::ugrpc::UnitTestServiceClient::GetMetadata();

    const ugrpc::client::RetryConfig retry_config;

    const auto service_name = metadata.service_full_name;

    const auto timeout0 = google::protobuf::util::TimeUtil::MillisecondsToDuration(100);
    const auto timeout2 = google::protobuf::util::TimeUtil::MillisecondsToDuration(500);
    const auto default_timeout = google::protobuf::util::TimeUtil::MillisecondsToDuration(1500);

    std::uint32_t max_attempts = 5;
    const auto retry_policy_json = BuildSimpleRetryPolicyConfig(max_attempts);

    const auto method0_config_json =
        BuildMethodConfig({{service_name, GetMethodName(metadata, 0)}}, timeout0, retry_policy_json);
    const auto method2_config_json =
        BuildMethodConfig({{service_name, GetMethodName(metadata, 2)}}, timeout2, retry_policy_json);
    const auto method3_config_json =
        BuildMethodConfig({{service_name, GetMethodName(metadata, 3)}}, std::nullopt, retry_policy_json);
    const auto default_method_config_json = BuildDefaultMethodConfig(default_timeout, retry_policy_json);

    const auto static_service_config = formats::json::MakeObject(
        "methodConfig",
        formats::json::MakeArray(
            method0_config_json, method2_config_json, method3_config_json, default_method_config_json
        )
    );
    LOG_DEBUG() << "static_service_config: " << static_service_config;

    const ugrpc::client::impl::compat::ServiceConfigBuilder service_config_builder{
        metadata, retry_config, formats::json::ToString(static_service_config)};

    const ugrpc::client::Qos qos0{/*attempts=*/2, /*timeout=*/std::nullopt};
    const ugrpc::client::Qos qos1{/*attempts=*/3, /*timeout=*/std::nullopt};
    const ugrpc::client::Qos qos3{/*attempts=*/std::nullopt, /*timeout=*/std::nullopt};
    const ugrpc::client::Qos qos_default{/*attempts=*/4, /*timeout=*/std::nullopt};
    ugrpc::client::ClientQos client_qos;
    client_qos.methods.Set(GetMethodFullName(metadata, 0), qos0);
    client_qos.methods.Set(GetMethodFullName(metadata, 1), qos1);
    client_qos.methods.Set(GetMethodFullName(metadata, 3), qos3);
    client_qos.methods.SetDefault(qos_default);

    const auto service_config = service_config_builder.Build(client_qos);
    LOG_DEBUG() << "service_config: " << service_config;

    ASSERT_TRUE(service_config.HasMember("methodConfig"));
    const auto& method_config = service_config["methodConfig"];
    ASSERT_TRUE(method_config.IsArray());
    ASSERT_EQ(5, method_config.GetSize());

    VerifyMethodConfig(
        method_config[0],
        {{service_name, GetMethodName(metadata, 0)}},
        /*timeout=*/timeout0,
        /*attempts=*/max_attempts
    );

    VerifyMethodConfig(
        method_config[1],
        {{service_name, GetMethodName(metadata, 1)}},
        /*timeout=*/default_timeout,
        /*attempts=*/qos1.attempts
    );

    VerifyMethodConfig(
        method_config[2],
        {{service_name, GetMethodName(metadata, 2)}},
        /*timeout=*/timeout2,
        /*attempts=*/qos_default.attempts
    );

    VerifyMethodConfig(
        method_config[3],
        {{service_name, GetMethodName(metadata, 3)}},
        /*timeout=*/std::nullopt,
        /*attempts=*/qos_default.attempts
    );

    VerifyMethodConfig(
        method_config[4],
        {{/*service_name=*/std::nullopt, /*method_name=*/std::nullopt}},
        /*timeout=*/default_timeout,
        /*attempts=*/max_attempts
    );
}

USERVER_NAMESPACE_END
