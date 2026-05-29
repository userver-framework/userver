#pragma once

#include <compare>
#include <optional>
#include <string>
#include <string_view>

#include <userver/utest/utest.hpp>

#include <userver/utils/from_string.hpp>
#include <userver/utils/regex.hpp>

#include <storages/redis/utest/impl/redis_connection_state.hpp>

USERVER_NAMESPACE_BEGIN

// This fixture is used for both Client and SubscribeClient tests, because
// Publish() method is part of Client - not SubscribeClient. And without
// Publish() there is no way to write proper unit tests for subscribing
template <class BaseRedisConnectionState>
class BaseRedisClientTestEx  // NOLINT(fuchsia-multiple-inheritance)
    : public ::testing::Test,
      public BaseRedisConnectionState {
public:
    struct Version {
        int major{};
        int minor{};
        int patch{};

        auto operator<=>(const Version&) const = default;
        bool operator==(const Version&) const = default;
    };

    enum class ServerKind : std::uint8_t {
        kRedis,
        kValkey,
    };

    void SetUp() override {
        auto info_reply = this->GetSentinel()->MakeRequest({"info", "server"}, "none", false).Get();
        ASSERT_TRUE(info_reply->IsOk());
        ASSERT_TRUE(info_reply->data.IsString());
        const auto info = info_reply->data.GetString();

        // redis_version is always present (valkey reports redis_version:7.2.x
        // for backwards compatibility, regardless of its own version)
        const utils::regex redis_version_regex(R"(redis_version:(\d+\.\d+\.\d+))");
        utils::match_results matches;
        ASSERT_TRUE(utils::regex_search(info, matches, redis_version_regex));
        redis_version_ = MakeVersion(matches[1]);

        // valkey_version is only present on valkey servers.
        const utils::regex valkey_version_regex(R"(valkey_version:(\d+\.\d+\.\d+))");
        if (utils::regex_search(info, matches, valkey_version_regex)) {
            valkey_version_ = MakeVersion(matches[1]);
            server_kind_ = ServerKind::kValkey;
        } else {
            server_kind_ = ServerKind::kRedis;
        }

        json_supported_ = ProbeJsonSupport();
    }

    bool CheckRedisVersion(Version since) const {
        const Version version = GetRedisVersion();
        return since <= version;
    }

    bool CheckValkeyVersion(Version since) const {
        const std::optional<Version> version_opt = GetValkeyVersion();

        if (!IsValkey() || !version_opt.has_value()) {
            ADD_FAILURE() << "Not a valkey server!";
            return false;
        }

        return since <= version_opt.value();
    }

    Version GetRedisVersion() const { return redis_version_; }
    std::optional<Version> GetValkeyVersion() const { return valkey_version_; }
    ServerKind GetServerKind() const { return server_kind_; }
    bool IsValkey() const { return server_kind_ == ServerKind::kValkey; }

    bool HasJsonCommands() const { return json_supported_; }

    static std::string SkipMsgByVersion(std::string_view command, Version version) {
        return fmt::format("{} command available since {}.{}.{}", command, version.major, version.minor, version.patch);
    }

    static std::string SkipMsgJsonUnsupported(std::string_view command) {
        return fmt::format(
            "{} requires JSON commands (RedisJSON / valkey-json) to be available on the server",
            command
        );
    }

private:
    bool ProbeJsonSupport() {
        auto reply = this->GetSentinel()->MakeRequest({"command", "info", "json.set"}, "none", false).Get();
        if (!reply->IsOk() || !reply->data.IsArray()) {
            return false;
        }
        const auto arr = reply->data.GetArray();
        // COMMAND INFO returns array of size N (one entry per requested command)
        // nil when command is unknown, and a non-empty array describing the command otherwise
        if (arr.empty()) {
            return false;
        }
        return !arr[0].IsNil();
    }

    static Version MakeVersion(std::string_view from) {
        const utils::regex rgx(R"((\d+)\.(\d+)\.(\d+))");
        utils::match_results matches;
        const auto result = utils::regex_search(from, matches, rgx);
        EXPECT_TRUE(result);
        if (!result) {
            return {};
        }
        return {
            utils::FromString<int>(matches[1]),
            utils::FromString<int>(matches[2]),
            utils::FromString<int>(matches[3])
        };
    }

    Version redis_version_;
    std::optional<Version> valkey_version_;
    ServerKind server_kind_{ServerKind::kRedis};
    bool json_supported_{false};
};

using BaseRedisClientTest = BaseRedisClientTestEx<storages::redis::utest::impl::RedisConnectionState>;

using BaseRedisClusterClientTest = BaseRedisClientTestEx<storages::redis::utest::impl::RedisClusterConnectionState>;

USERVER_NAMESPACE_END
