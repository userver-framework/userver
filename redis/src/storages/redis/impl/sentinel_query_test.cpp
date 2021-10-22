#include "sentinel_query.hpp"
#include <gtest/gtest.h>
#include <userver/storages/redis/impl/reply.hpp>

#include <hiredis/hiredis.h>

USERVER_NAMESPACE_BEGIN

using namespace USERVER_NAMESPACE::redis;

TEST(SentinelQuery, SingleBadReply) {
  SentinelInstanceResponse resp;

  int called = 0;
  int size = 0;

  auto cb = [&](const ConnInfoByShard& info, size_t, size_t) {
    called++;
    size = info.size();
  };
  auto context =
      std::make_shared<GetHostsContext>(true, redis::Password("pass"), cb, 1);

  auto reply = std::make_shared<Reply>("cmd", ReplyData("str"));
  context->GenerateCallback()(nullptr, reply);

  EXPECT_EQ(0, size);
  EXPECT_EQ(1, called);
}

std::shared_ptr<Reply> GenerateReply(const std::string& ip, bool master,
                                     bool s_down, bool o_down,
                                     bool master_link_status_err) {
  std::string flags = master ? "master" : "slave";
  if (s_down) flags += ",s_down";
  if (o_down) flags += ",o_down";

  std::vector<ReplyData> array{
      {"flags"}, {std::move(flags)}, {"name"}, {"inst-name"}, {"ip"},
      {ip},      {"port"},           {"1111"}};
  if (!master) {
    array.emplace_back("master-link-status");
    array.emplace_back(master_link_status_err ? "err" : "ok");
  }

  std::vector<ReplyData> array2{ReplyData(std::move(array))};
  return std::make_shared<Reply>("cmd", ReplyData(std::move(array2)));
}

const auto kHost1 = "127.0.0.1";
const auto kHost2 = "127.0.0.2";

TEST(SentinelQuery, SingleOkReply) {
  SentinelInstanceResponse resp;

  int called = 0;
  int size = 0;

  auto cb = [&](const ConnInfoByShard& info, size_t, size_t) {
    called++;
    size = info.size();
  };
  auto context =
      std::make_shared<GetHostsContext>(true, redis::Password("pass"), cb, 1);

  auto reply = GenerateReply(kHost1, false, false, false, false);
  context->GenerateCallback()(nullptr, reply);

  EXPECT_EQ(1, size);
  EXPECT_EQ(1, called);
}

TEST(SentinelQuery, SingleSDownReply) {
  SentinelInstanceResponse resp;

  int called = 0;
  int size = 0;

  auto cb = [&](const ConnInfoByShard& info, size_t, size_t) {
    called++;
    size = info.size();
  };
  auto context =
      std::make_shared<GetHostsContext>(true, redis::Password("pass"), cb, 1);

  auto reply = GenerateReply(kHost2, false, true, false, false);
  context->GenerateCallback()(nullptr, reply);

  EXPECT_EQ(0, size);
  EXPECT_EQ(1, called);
}

TEST(SentinelQuery, MultipleOkOkOk) {
  SentinelInstanceResponse resp;

  int called = 0;
  int size = 0;

  auto cb = [&](const ConnInfoByShard& info, size_t, size_t) {
    called++;
    size = info.size();
  };
  auto context =
      std::make_shared<GetHostsContext>(1, redis::Password("pass"), cb, 3);

  auto reply = GenerateReply(kHost1, false, false, false, false);

  context->GenerateCallback()(nullptr, reply);
  EXPECT_EQ(0, called);

  context->GenerateCallback()(nullptr, reply);
  EXPECT_EQ(0, called);

  context->GenerateCallback()(nullptr, reply);
  EXPECT_EQ(1, size);
  EXPECT_EQ(1, called);
}

TEST(SentinelQuery, MultipleOkOkMastererr) {
  SentinelInstanceResponse resp;

  int called = 0;
  int size = 0;

  auto cb = [&](const ConnInfoByShard& info, size_t, size_t) {
    called++;
    size = info.size();
  };
  auto context =
      std::make_shared<GetHostsContext>(1, redis::Password("pass"), cb, 3);

  auto reply = GenerateReply(kHost1, false, false, false, false);
  context->GenerateCallback()(nullptr, reply);
  EXPECT_EQ(0, called);

  context->GenerateCallback()(nullptr, reply);
  EXPECT_EQ(0, called);

  reply = GenerateReply(kHost1, false, false, false, true);
  context->GenerateCallback()(nullptr, reply);
  EXPECT_EQ(1, size);
  EXPECT_EQ(1, called);
}

TEST(SentinelQuery, MultipleOkMastererrMastererr) {
  SentinelInstanceResponse resp;

  int called = 0;
  int size = 0;

  auto cb = [&](const ConnInfoByShard& info, size_t, size_t) {
    called++;
    size = info.size();
  };
  auto context =
      std::make_shared<GetHostsContext>(1, redis::Password("pass"), cb, 3);

  auto reply = GenerateReply(kHost1, false, false, false, false);
  context->GenerateCallback()(nullptr, reply);
  EXPECT_EQ(0, called);

  reply = GenerateReply(kHost1, false, false, false, true);
  context->GenerateCallback()(nullptr, reply);
  EXPECT_EQ(0, called);

  context->GenerateCallback()(nullptr, reply);
  EXPECT_EQ(0, size);
  EXPECT_EQ(1, called);
}

TEST(SentinelQuery, MultipleOkOkSDown) {
  SentinelInstanceResponse resp;

  int called = 0;
  int size = 0;

  auto cb = [&](const ConnInfoByShard& info, size_t, size_t) {
    called++;
    size = info.size();
  };
  auto context =
      std::make_shared<GetHostsContext>(1, redis::Password("pass"), cb, 3);

  auto reply = GenerateReply(kHost1, false, false, false, false);
  context->GenerateCallback()(nullptr, reply);
  EXPECT_EQ(0, called);

  context->GenerateCallback()(nullptr, reply);
  EXPECT_EQ(0, called);

  reply = GenerateReply(kHost1, false, true, false, false);
  context->GenerateCallback()(nullptr, reply);
  EXPECT_EQ(1, size);
  EXPECT_EQ(1, called);
}

TEST(SentinelQuery, MultipleOkSDownSDown) {
  SentinelInstanceResponse resp;

  int called = 0;
  int size = 0;

  auto cb = [&](const ConnInfoByShard& info, size_t, size_t) {
    called++;
    size = info.size();
  };
  auto context =
      std::make_shared<GetHostsContext>(1, redis::Password("pass"), cb, 3);

  auto reply = GenerateReply(kHost1, false, false, false, false);
  context->GenerateCallback()(nullptr, reply);
  EXPECT_EQ(0, called);

  reply = GenerateReply(kHost1, false, true, false, false);
  context->GenerateCallback()(nullptr, reply);
  EXPECT_EQ(0, called);

  context->GenerateCallback()(nullptr, reply);
  EXPECT_EQ(0, size);
  EXPECT_EQ(1, called);
}

TEST(SentinelQuery, MultipleOkOkODown) {
  SentinelInstanceResponse resp;

  int called = 0;
  int size = 0;

  auto cb = [&](const ConnInfoByShard& info, size_t, size_t) {
    called++;
    size = info.size();
  };
  auto context =
      std::make_shared<GetHostsContext>(1, redis::Password("pass"), cb, 3);

  auto reply = GenerateReply(kHost1, false, false, false, false);
  context->GenerateCallback()(nullptr, reply);
  EXPECT_EQ(0, called);

  context->GenerateCallback()(nullptr, reply);
  EXPECT_EQ(0, called);

  reply = GenerateReply(kHost1, false, false, true, false);
  context->GenerateCallback()(nullptr, reply);
  EXPECT_EQ(0, size);
  EXPECT_EQ(1, called);
}

TEST(SentinelQuery, DifferentAnswers1) {
  SentinelInstanceResponse resp;

  int called = 0;

  auto cb = [&](const ConnInfoByShard& info, size_t, size_t) {
    called++;
    ASSERT_EQ(info.size(), 1u);
    auto shard_info = info[0];
    EXPECT_EQ(shard_info.host, kHost1);
  };
  auto context =
      std::make_shared<GetHostsContext>(1, redis::Password("pass"), cb, 3);

  auto reply = GenerateReply(kHost1, true, false, false, false);
  context->GenerateCallback()(nullptr, reply);
  context->GenerateCallback()(nullptr, reply);
  reply = GenerateReply(kHost2, true, false, false, false);
  context->GenerateCallback()(nullptr, reply);
  EXPECT_EQ(1, called);
}

TEST(SentinelQuery, DifferentAnswers2) {
  SentinelInstanceResponse resp;

  int called = 0;

  auto cb = [&](const ConnInfoByShard& info, size_t, size_t) {
    called++;
    ASSERT_EQ(info.size(), 1u);
    auto shard_info = info[0];
    EXPECT_EQ(shard_info.host, kHost1);
  };
  auto context =
      std::make_shared<GetHostsContext>(1, redis::Password("pass"), cb, 3);

  auto reply = GenerateReply(kHost2, true, false, false, false);
  context->GenerateCallback()(nullptr, reply);
  reply = GenerateReply(kHost1, true, false, false, false);
  context->GenerateCallback()(nullptr, reply);
  context->GenerateCallback()(nullptr, reply);
  EXPECT_EQ(1, called);
}

USERVER_NAMESPACE_END
