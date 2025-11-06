#include <string>

#include <etcd/client_impl.hpp>
#include <userver/crypto/base64.hpp>
#include <userver/etcd/client.hpp>
#include <userver/etcd/settings.hpp>
#include <userver/formats/json/inline.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/utest/http_client.hpp>
#include <userver/utest/http_server_mock.hpp>
#include <userver/utest/utest.hpp>
#include <userver/utils/async.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

utest::HttpServerMock::HttpResponse EtcdRequestProcessor(const utest::HttpServerMock::HttpRequest& request) {
    static std::map<std::string, etcd::KeyValueState> storage;

    EXPECT_EQ(request.method, clients::http::HttpMethod::kPost);
    const auto request_body = formats::json::FromString(request.body);
    formats::json::ValueBuilder response_body_value_builder;
    const auto key = crypto::base64::Base64Decode(request_body["key"].As<std::string>());

    if (request.path == "/v3/kv/put") {
        const auto value = crypto::base64::Base64Decode(request_body["value"].As<std::string>());
        int32_t new_version = 1;
        const auto key_value_iterator = storage.find(key);
        if (key_value_iterator != storage.end()) {
            new_version = (key_value_iterator->second).version + 1;
        }
        etcd::KeyValueState key_value_state;
        key_value_state.key = key;
        key_value_state.value = value;
        key_value_state.version = new_version;
        storage[key] = key_value_state;

    } else if (request.path == "/v3/kv/range" && !request_body.HasMember("range_end")) {
        const auto value_iterator = storage.find(key);
        response_body_value_builder["kvs"] = formats::json::MakeArray();
        if (value_iterator != storage.end()) {
            response_body_value_builder["kvs"].PushBack(formats::json::MakeObject(
                "key",
                crypto::base64::Base64Encode((value_iterator->second).key),
                "value",
                crypto::base64::Base64Encode((value_iterator->second).value),
                "version",
                std::to_string((value_iterator->second).version)
            ));
        }
    } else if (request.path == "/v3/kv/range") {
        const auto range_end = crypto::base64::Base64Decode(request_body["range_end"].As<std::string>());
        auto first_key = storage.lower_bound(key);
        const auto last_key = storage.upper_bound(range_end);

        response_body_value_builder["kvs"] = formats::json::MakeArray();
        while (first_key != last_key) {
            response_body_value_builder["kvs"].PushBack(formats::json::MakeObject(
                "key",
                crypto::base64::Base64Encode((first_key->second).key),
                "value",
                crypto::base64::Base64Encode((first_key->second).value),
                "version",
                std::to_string((first_key->second).version)
            ));
            ++first_key;
        }
    } else if (request.path == "/v3/kv/deleterange") {
        storage.erase(key);
    }

    return utest::HttpServerMock::HttpResponse{
        200, clients::http::Headers{}, formats::json::ToString(response_body_value_builder.ExtractValue())};
}

}  // namespace

UTEST(Etcd, TestKeyValueStorage) {
    utest::HttpServerMock mock_server(&EtcdRequestProcessor);
    auto http_client_ptr = utest::CreateHttpClient();
    auto etcd_client_ptr = std::make_shared<etcd::impl::ClientImpl>(
        *http_client_ptr,
        etcd::ClientSettings{
            {mock_server.GetBaseUrl()},
            2,
            std::chrono::milliseconds{500},
            std::chrono::milliseconds{100'000},
        }
    );

    const auto empty_value = etcd_client_ptr->Get("key_with_empty_value");
    EXPECT_EQ(empty_value, std::nullopt);

    etcd_client_ptr->Put("some_key", "some_value");
    const auto some_value = etcd_client_ptr->Get("some_key");
    EXPECT_EQ(some_value, "some_value");

    etcd_client_ptr->Put("some_key", "some_new_value");
    const auto some_new_value = etcd_client_ptr->Get("some_key");
    EXPECT_EQ(some_new_value, "some_new_value");

    etcd_client_ptr->Delete("some_key");
    const auto deleted_value = etcd_client_ptr->Get("some_key");
    EXPECT_EQ(deleted_value, std::nullopt);
}

UTEST(Etcd, TestRange) {
    utest::HttpServerMock mock_server(&EtcdRequestProcessor);
    auto http_client_ptr = utest::CreateHttpClient();
    auto etcd_client_ptr = std::make_shared<etcd::impl::ClientImpl>(
        *http_client_ptr,
        etcd::ClientSettings{
            {mock_server.GetBaseUrl()},
            2,
            std::chrono::milliseconds{500},
            std::chrono::milliseconds{100'000},
        }
    );
    const uint32_t range_size = 3;

    EXPECT_TRUE(etcd_client_ptr->Range("some_key").empty());

    for (uint32_t i = 1; i <= range_size; ++i) {
        etcd_client_ptr->Put(fmt::format("some_key_{}", i), fmt::format("some_value_{}", i));
    }

    auto range_result = etcd_client_ptr->Range("some_key");
    EXPECT_EQ(range_result.size(), range_size);
    std::sort(range_result.begin(), range_result.end(), [](const etcd::KeyValueState& l, const etcd::KeyValueState& r) {
        return l.value < r.value;
    });

    for (uint32_t i = 1; i <= range_size; ++i) {
        EXPECT_EQ(range_result[i - 1].value, fmt::format("some_value_{}", i));
    }

    for (uint32_t i = 1; i <= range_size; ++i) {
        etcd_client_ptr->Delete(fmt::format("some_key_{}", i));
    }

    EXPECT_TRUE(etcd_client_ptr->Range("some_key").empty());
}

USERVER_NAMESPACE_END
