#include <storages/redis/client_cluster_redistest.hpp>

#include <array>

#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value_builder.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

const storages::redis::CommandControl kDefaultCc{
    std::chrono::milliseconds(300),
    std::chrono::milliseconds(300),
    1,
};

/// Use this CommandControl for read commands that follow a write in tests,
/// to avoid replication lag causing flaky failures.
const storages::redis::CommandControl kMasterCC = [] {
    auto cc = kDefaultCc;
    cc.force_request_to_master = true;
    return cc;
}();

struct FanId {
    std::string id;
    std::string name;
    std::string nickname;
    std::string football_club;
};

const FanId kNikita{
    .id = "{fan_id}:nikita",
    .name = "Nikita Vtorushin",
    .nickname = "grandmaster",
    .football_club = "Nottingham Forest",
};
const FanId kPavel{
    .id = "{fan_id}:pavel",
    .name = "Pavel Popov",
    .nickname = "tiger",
    .football_club = "Millwall",
};
const FanId kAlexey{
    .id = "{fan_id}:alexey",
    .name = "Alexey Bykov",
    .nickname = "legacy enjoyer",
    .football_club = "Arsenal",
};
const FanId kDmitry{
    .id = "{fan_id}:dima",
    .name = "Dmitry Kazantsev",
    .nickname = "repl1cant",
    .football_club = "Spartak",
};

formats::json::Value FanToJson(const FanId& fan) {
    formats::json::ValueBuilder b;
    b["name"] = fan.name;
    b["nickname"] = fan.nickname;
    b["fc"] = fan.football_club;
    return b.ExtractValue();
}

}  // namespace

UTEST_F(RedisClusterClientTest, JsonSetGet) {
    if (!HasJsonCommands()) {
        GTEST_SKIP() << SkipMsgJsonUnsupported("JsonSetGet");
    }

    auto client = GetClient();

    const auto& fan = kNikita;
    const auto fan_json = FanToJson(fan);

    // JSON.SET — store profile at root
    UASSERT_NO_THROW(client->JsonSet(fan.id, "$", fan_json, kDefaultCc).Get());

    // JSON.GET root path — returns full document
    {
        auto reply = client->JsonGet(fan.id, kMasterCC).Get();
        ASSERT_TRUE(reply);
        EXPECT_EQ(reply.value(), fan_json);
    }

    // JSON.GET single JSONPath - returns array of matches
    {
        auto reply = client->JsonGet(fan.id, "$.nickname", kMasterCC).Get();
        ASSERT_TRUE(reply);
        formats::json::ValueBuilder expected;
        expected.PushBack(std::string{fan.nickname});
        EXPECT_EQ(reply.value(), expected.ExtractValue());
    }

    // JSON.GET multiple JSONPaths — returns an object keyed by the path
    // expression itself, with each value being an array of matches
    {
        auto reply = client->JsonGet(fan.id, std::vector<std::string>{"$.name", "$.fc"}, kMasterCC).Get();
        ASSERT_TRUE(reply);

        formats::json::ValueBuilder expected;

        formats::json::ValueBuilder name_arr;
        name_arr.PushBack(std::string{fan.name});
        expected["$.name"] = name_arr.ExtractValue();

        formats::json::ValueBuilder fc_arr;
        fc_arr.PushBack(std::string{fan.football_club});
        expected["$.fc"] = fc_arr.ExtractValue();

        EXPECT_EQ(reply.value(), expected.ExtractValue());
    }

    // Missing key — std::nullopt empty
    {
        auto reply = client->JsonGet("{fan_id}:nobody", kMasterCC).Get();
        EXPECT_FALSE(reply.has_value());
    }
}

UTEST_F(RedisClusterClientTest, JsonSetIfNotExist) {
    if (!HasJsonCommands()) {
        GTEST_SKIP() << SkipMsgJsonUnsupported("JsonSetIfNotExist");
    }

    auto client = GetClient();

    const auto& fan = kPavel;
    const auto original_json = FanToJson(fan);

    // First NX write should succeed — key not exist
    {
        auto reply = client->JsonSetIfNotExist(fan.id, "$", original_json, kDefaultCc).Get();
        ASSERT_TRUE(reply);
    }

    // Second NX write must NOT overwrite Pavel with Alexey
    {
        const auto intruder_json = FanToJson(kAlexey);
        auto reply = client->JsonSetIfNotExist(fan.id, "$", intruder_json, kDefaultCc).Get();
        EXPECT_FALSE(reply);
    }

    // The original profile must survive
    {
        auto reply = client->JsonGet(fan.id, kMasterCC).Get();
        ASSERT_TRUE(reply);
        EXPECT_EQ(reply.value(), original_json);
    }
}

UTEST_F(RedisClusterClientTest, JsonSetIfExist) {
    if (!HasJsonCommands()) {
        GTEST_SKIP() << SkipMsgJsonUnsupported("JsonSetIfExist");
    }

    auto client = GetClient();

    const auto& fan = kAlexey;
    const auto original_json = FanToJson(fan);

    // XX on a missing key must fail
    {
        auto reply = client->JsonSetIfExist(fan.id, "$", original_json, kDefaultCc).Get();
        EXPECT_FALSE(reply);
    }

    // Seed the key, then overwrite Alexey with Nikita using XX
    UASSERT_NO_THROW(client->JsonSet(fan.id, "$", original_json, kDefaultCc).Get());

    {
        const auto new_json = FanToJson(kNikita);
        auto reply = client->JsonSetIfExist(fan.id, "$", new_json, kDefaultCc).Get();
        ASSERT_TRUE(reply);
    }

    {
        auto reply = client->JsonGet(fan.id, kMasterCC).Get();
        ASSERT_TRUE(reply);
        EXPECT_EQ(reply.value()["nickname"].As<std::string>(), kNikita.nickname);
        EXPECT_EQ(reply.value()["fc"].As<std::string>(), kNikita.football_club);
    }
}

UTEST_F(RedisClusterClientTest, JsonMgetMset) {
    if (!HasJsonCommands()) {
        GTEST_SKIP() << SkipMsgJsonUnsupported("JsonMgetMset");
    }

    auto client = GetClient();

    // All keys share the '{fan_id}' hash tag so MSET stays on a single slot
    const std::array<FanId, 4> fans{kNikita, kPavel, kAlexey, kDmitry};

    std::vector<storages::redis::JsonKeyPathValue> entries;
    entries.reserve(fans.size());
    for (const auto& fan : fans) {
        entries.push_back({fan.id, "$", FanToJson(fan)});
    }

    UASSERT_NO_THROW(client->JsonMset(entries, kDefaultCc).Get());

    // Read back the favourite club for each fan via JSON.MGET $.fc
    {
        std::vector<std::string> keys;
        keys.reserve(fans.size());
        for (const auto& fan : fans) {
            keys.push_back(fan.id);
        }

        auto reply = client->JsonMget(keys, "$.fc", kMasterCC).Get();
        ASSERT_EQ(reply.size(), fans.size());

        for (size_t i = 0; i < fans.size(); ++i) {
            ASSERT_TRUE(reply[i]) << "missing reply for " << fans[i].id;
            // JSON.MGET with a JSONPath returns an array of matches per key
            ASSERT_TRUE(reply[i].value().IsArray()) << "expected array of matches for " << fans[i].id;
            ASSERT_EQ(reply[i].value().GetSize(), 1U);
            EXPECT_EQ(reply[i].value()[0].As<std::string>(), fans[i].football_club);
        }
    }

    // JSON.MGET with no keys — sanity check for empty input
    {
        auto reply = client->JsonMget(std::vector<std::string>{}, "$", kMasterCC).Get();
        EXPECT_TRUE(reply.empty());
    }
}

UTEST_F(RedisClusterClientTest, JsonTransaction) {
    if (!HasJsonCommands()) {
        GTEST_SKIP() << SkipMsgJsonUnsupported("JsonTransaction");
    }

    auto client = GetClient();

    auto transaction = client->Multi();

    // Same '{fan_id}' hash tag = guaranteed same shard, MULTI/EXEC is legal
    const auto nikita_json = FanToJson(kNikita);
    const auto pavel_json = FanToJson(kPavel);

    auto set1 = transaction->JsonSet(kNikita.id, "$", nikita_json);
    auto get1 = transaction->JsonGet(kNikita.id);
    auto set2 = transaction->JsonSet(kPavel.id, "$", pavel_json);
    auto get2 = transaction->JsonGet(kPavel.id);

    UASSERT_NO_THROW(transaction->Exec(kDefaultCc).Get());

    auto reply1 = get1.Get();
    ASSERT_TRUE(reply1);
    EXPECT_EQ(reply1.value()["nickname"].As<std::string>(), kNikita.nickname);
    EXPECT_EQ(reply1.value()["fc"].As<std::string>(), kNikita.football_club);

    auto reply2 = get2.Get();
    ASSERT_TRUE(reply2);
    EXPECT_EQ(reply2.value()["nickname"].As<std::string>(), kPavel.nickname);
    EXPECT_EQ(reply2.value()["fc"].As<std::string>(), kPavel.football_club);
}

USERVER_NAMESPACE_END
