#include <userver/utest/utest.hpp>

#include <userver/congestion_control/controllers/linear.hpp>
#include <userver/dynamic_config/test_helpers.hpp>

USERVER_NAMESPACE_BEGIN

class FakeSensor : public congestion_control::v2::Sensor {
  Data GetCurrent() override { return {}; }
};

class FakeLimiter : public congestion_control::Limiter {
  void SetLimit(const congestion_control::Limit&) override {}
};

namespace {
congestion_control::v2::Stats stats;
FakeSensor sensor;
FakeLimiter limiter;
}  // namespace

TEST(CCLinear, Zero) {
  congestion_control::v2::LinearController controller(
      "test", sensor, limiter, stats, {}, dynamic_config::GetDefaultSource(),
      [](auto) { return congestion_control::v2::Config(); });

  for (size_t i = 0; i < 1000; i++) {
    auto limit = controller.Update({});
    EXPECT_EQ(limit.load_limit, std::nullopt) << i;
  }
}

TEST(CCLinear, FirstSeconds) {
  congestion_control::v2::LinearController controller(
      "test", sensor, limiter, stats, {}, dynamic_config::GetDefaultSource(),
      [](auto) { return congestion_control::v2::Config(); });

  for (size_t i = 0; i < 30; i++) {
    congestion_control::v2::Sensor::Data data;
    data.timings_avg_ms = 10000;
    data.total = 100;

    auto limit = controller.Update(data);
    EXPECT_EQ(limit.load_limit, std::nullopt) << i;
  }
}

TEST(CCLinear, SmallRps) {
  congestion_control::v2::LinearController controller(
      "test", sensor, limiter, stats, {}, dynamic_config::GetDefaultSource(),
      [](auto) { return congestion_control::v2::Config(); });
  // First seconds
  for (size_t i = 0; i < 30; i++) {
    congestion_control::v2::Sensor::Data data;
    data.timings_avg_ms = 10000;
    data.total = 100;

    auto limit = controller.Update(data);
    EXPECT_EQ(limit.load_limit, std::nullopt) << i;
  }

  // Small RPS
  for (size_t i = 0; i < 100; i++) {
    congestion_control::v2::Sensor::Data data;
    data.timings_avg_ms = 10000;
    data.total = 1;

    auto limit = controller.Update(data);
    EXPECT_EQ(limit.load_limit, std::nullopt) << i;
  }
}

TEST(CCLinear, SmallSpike) {
  congestion_control::v2::LinearController controller(
      "test", sensor, limiter, stats, {}, dynamic_config::GetDefaultSource(),
      [](auto) { return congestion_control::v2::Config(); });
  // First seconds
  for (size_t i = 0; i < 30; i++) {
    congestion_control::v2::Sensor::Data data;
    data.timings_avg_ms = 100;
    data.total = 100;

    auto limit = controller.Update(data);
    EXPECT_EQ(limit.load_limit, std::nullopt) << i;
  }

  // Extra Load
  congestion_control::v2::Sensor::Data data;
  data.timings_avg_ms = 4000;
  data.total = 1;

  auto limit = controller.Update(data);
  EXPECT_EQ(limit.load_limit, std::nullopt) << 0;

  // Back to normal
  data.timings_avg_ms = 100;
  limit = controller.Update(data);
  EXPECT_EQ(limit.load_limit, std::nullopt) << 1;

  limit = controller.Update(data);
  EXPECT_EQ(limit.load_limit, std::nullopt) << 2;

  limit = controller.Update(data);
  EXPECT_EQ(limit.load_limit, std::nullopt) << 3;
}

TEST(CCLinear, ExtraLoad) {
  congestion_control::v2::LinearController controller(
      "test", sensor, limiter, stats, {}, dynamic_config::GetDefaultSource(),
      [](auto) { return congestion_control::v2::Config(); });

  // Init
  for (size_t i = 0; i < 31; i++) {
    congestion_control::v2::Sensor::Data data;
    data.timings_avg_ms = 100;
    data.total = 100;

    auto limit = controller.Update(data);
    EXPECT_EQ(limit.load_limit, std::nullopt) << i;
  }

  // Extra load
  for (size_t i = 0; i < 2; i++) {
    congestion_control::v2::Sensor::Data data;
    data.timings_avg_ms = 5001;
    data.total = 100;

    controller.Update(data);
  }
  for (size_t i = 0; i < 100; i++) {
    congestion_control::v2::Sensor::Data data;
    data.timings_avg_ms = 5001;
    data.total = 100;

    auto limit = controller.Update(data);
    EXPECT_NE(limit.load_limit, std::nullopt) << i;
  }

  // Back to normal
  for (size_t i = 0; i < 100; i++) {
    congestion_control::v2::Sensor::Data data;
    data.timings_avg_ms = 100;
    data.total = 100;

    controller.Update(data);
  }

  // Normal
  for (size_t i = 0; i < 1000; i++) {
    congestion_control::v2::Sensor::Data data;
    data.timings_avg_ms = 100;
    data.total = 100;

    auto limit = controller.Update(data);
    EXPECT_EQ(limit.load_limit, std::nullopt) << i;
  }
}

TEST(CCLinear, MinMax) {
  congestion_control::v2::LinearController controller(
      "test", sensor, limiter, stats, {}, dynamic_config::GetDefaultSource(),
      [](auto) { return congestion_control::v2::Config(); });
  // First seconds
  for (size_t i = 0; i < 30; i++) {
    congestion_control::v2::Sensor::Data data;
    data.timings_avg_ms = 0;
    data.total = 100;

    auto limit = controller.Update(data);
    EXPECT_EQ(limit.load_limit, std::nullopt) << i;
  }

  // Extra Load
  congestion_control::v2::Sensor::Data data;
  data.timings_avg_ms = 0;
  data.total = 100;

  auto limit = controller.Update(data);
  EXPECT_EQ(limit.load_limit, std::nullopt) << 0;

  limit = controller.Update(data);
  EXPECT_EQ(limit.load_limit, std::nullopt) << 0;

  limit = controller.Update(data);
  EXPECT_EQ(limit.load_limit, std::nullopt) << 0;

  data.timings_avg_ms = 1;

  limit = controller.Update(data);
  EXPECT_EQ(limit.load_limit, std::nullopt) << 0;

  limit = controller.Update(data);
  EXPECT_EQ(limit.load_limit, std::nullopt) << 0;

  limit = controller.Update(data);
  EXPECT_EQ(limit.load_limit, std::nullopt) << 0;
}

USERVER_NAMESPACE_END
