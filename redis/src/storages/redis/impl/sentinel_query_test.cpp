#include <gtest/gtest.h>
#include <storages/redis/impl/sentinel_query.hpp>
#include <userver/storages/redis/impl/reply.hpp>

#include <hiredis/hiredis.h>

USERVER_NAMESPACE_BEGIN

TEST(SentinelQuery, SingleBadReply) {
  redis::SentinelInstanceResponse resp;

  int called = 0;
  int size = 0;

  auto cb = [&](const redis::ConnInfoByShard& info, size_t, size_t) {
    called++;
    size = info.size();
  };
  auto context = std::make_shared<redis::GetHostsContext>(
      true, redis::Password("pass"), cb, 1);

  auto reply = std::make_shared<redis::Reply>("cmd", redis::ReplyData("str"));
  context->GenerateCallback()(nullptr, reply);

  EXPECT_EQ(0, size);
  EXPECT_EQ(1, called);
}

std::shared_ptr<redis::Reply> GenerateReply(const std::string& ip, bool master,
                                            bool s_down, bool o_down,
                                            bool master_link_status_err) {
  std::string flags = master ? "master" : "slave";
  if (s_down) flags += ",s_down";
  if (o_down) flags += ",o_down";

  std::vector<redis::ReplyData> array{
      {"flags"}, {std::move(flags)}, {"name"}, {"inst-name"}, {"ip"},
      {ip},      {"port"},           {"1111"}};
  if (!master) {
    array.emplace_back("master-link-status");
    array.emplace_back(master_link_status_err ? "err" : "ok");
  }

  std::vector<redis::ReplyData> array2{redis::ReplyData(std::move(array))};
  return std::make_shared<redis::Reply>("cmd",
                                        redis::ReplyData(std::move(array2)));
}

const auto kHost1 = "127.0.0.1";
const auto kHost2 = "127.0.0.2";

TEST(SentinelQuery, SingleOkReply) {
  redis::SentinelInstanceResponse resp;

  int called = 0;
  int size = 0;

  auto cb = [&](const redis::ConnInfoByShard& info, size_t, size_t) {
    called++;
    size = info.size();
  };
  auto context = std::make_shared<redis::GetHostsContext>(
      true, redis::Password("pass"), cb, 1);

  auto reply = GenerateReply(kHost1, false, false, false, false);
  context->GenerateCallback()(nullptr, reply);

  EXPECT_EQ(1, size);
  EXPECT_EQ(1, called);
}

TEST(SentinelQuery, SingleSDownReply) {
  redis::SentinelInstanceResponse resp;

  int called = 0;
  int size = 0;

  auto cb = [&](const redis::ConnInfoByShard& info, size_t, size_t) {
    called++;
    size = info.size();
  };
  auto context = std::make_shared<redis::GetHostsContext>(
      true, redis::Password("pass"), cb, 1);

  auto reply = GenerateReply(kHost2, false, true, false, false);
  context->GenerateCallback()(nullptr, reply);

  EXPECT_EQ(0, size);
  EXPECT_EQ(1, called);
}

TEST(SentinelQuery, MultipleOkOkOk) {
  redis::SentinelInstanceResponse resp;

  int called = 0;
  int size = 0;

  auto cb = [&](const redis::ConnInfoByShard& info, size_t, size_t) {
    called++;
    size = info.size();
  };
  auto context = std::make_shared<redis::GetHostsContext>(
      1, redis::Password("pass"), cb, 3);

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
  redis::SentinelInstanceResponse resp;

  int called = 0;
  int size = 0;

  auto cb = [&](const redis::ConnInfoByShard& info, size_t, size_t) {
    called++;
    size = info.size();
  };
  auto context = std::make_shared<redis::GetHostsContext>(
      1, redis::Password("pass"), cb, 3);

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
  redis::SentinelInstanceResponse resp;

  int called = 0;
  int size = 0;

  auto cb = [&](const redis::ConnInfoByShard& info, size_t, size_t) {
    called++;
    size = info.size();
  };
  auto context = std::make_shared<redis::GetHostsContext>(
      1, redis::Password("pass"), cb, 3);

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
  redis::SentinelInstanceResponse resp;

  int called = 0;
  int size = 0;

  auto cb = [&](const redis::ConnInfoByShard& info, size_t, size_t) {
    called++;
    size = info.size();
  };
  auto context = std::make_shared<redis::GetHostsContext>(
      1, redis::Password("pass"), cb, 3);

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
  redis::SentinelInstanceResponse resp;

  int called = 0;
  int size = 0;

  auto cb = [&](const redis::ConnInfoByShard& info, size_t, size_t) {
    called++;
    size = info.size();
  };
  auto context = std::make_shared<redis::GetHostsContext>(
      1, redis::Password("pass"), cb, 3);

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
  redis::SentinelInstanceResponse resp;

  int called = 0;
  int size = 0;

  auto cb = [&](const redis::ConnInfoByShard& info, size_t, size_t) {
    called++;
    size = info.size();
  };
  auto context = std::make_shared<redis::GetHostsContext>(
      1, redis::Password("pass"), cb, 3);

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
  redis::SentinelInstanceResponse resp;

  int called = 0;

  auto cb = [&](const redis::ConnInfoByShard& info, size_t, size_t) {
    called++;
    ASSERT_EQ(info.size(), 1u);
    const auto& shard_info = info[0];
    EXPECT_EQ(shard_info.HostPort().first, kHost1);
  };
  auto context = std::make_shared<redis::GetHostsContext>(
      1, redis::Password("pass"), cb, 3);

  auto reply = GenerateReply(kHost1, true, false, false, false);
  context->GenerateCallback()(nullptr, reply);
  context->GenerateCallback()(nullptr, reply);
  reply = GenerateReply(kHost2, true, false, false, false);
  context->GenerateCallback()(nullptr, reply);
  EXPECT_EQ(1, called);
}

TEST(SentinelQuery, DifferentAnswers2) {
  redis::SentinelInstanceResponse resp;

  int called = 0;

  auto cb = [&](const redis::ConnInfoByShard& info, size_t, size_t) {
    called++;
    ASSERT_EQ(info.size(), 1u);
    const auto& shard_info = info[0];
    EXPECT_EQ(shard_info.HostPort().first, kHost1);
  };
  auto context = std::make_shared<redis::GetHostsContext>(
      1, redis::Password("pass"), cb, 3);

  auto reply = GenerateReply(kHost2, true, false, false, false);
  context->GenerateCallback()(nullptr, reply);
  reply = GenerateReply(kHost1, true, false, false, false);
  context->GenerateCallback()(nullptr, reply);
  context->GenerateCallback()(nullptr, reply);
  EXPECT_EQ(1, called);
}

USERVER_NAMESPACE_END
