#include <userver/utest/utest.hpp>

#include <userver/congestion_control/controllers/linear.hpp>

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
  congestion_control::v2::LinearController controller("test", sensor, limiter,
                                                      stats, {});

  for (size_t i = 0; i < 1000; i++) {
    auto limit = controller.Update({});
    EXPECT_EQ(limit.load_limit, std::nullopt) << i;
  }
}

TEST(CCLinear, FirstSeconds) {
  congestion_control::v2::LinearController controller("test", sensor, limiter,
                                                      stats, {});

  for (size_t i = 0; i < 30; i++) {
    congestion_control::v2::Sensor::Data data;
    data.timings_avg_ms = 10000;
    data.total = 100;

    auto limit = controller.Update(data);
    EXPECT_EQ(limit.load_limit, std::nullopt) << i;
  }
}

TEST(CCLinear, ExtraLoad) {
  congestion_control::v2::LinearController controller("test", sensor, limiter,
                                                      stats, {});

  // Init
  for (size_t i = 0; i < 31; i++) {
    congestion_control::v2::Sensor::Data data;
    data.timings_avg_ms = 100;
    data.total = 100;

    auto limit = controller.Update(data);
    EXPECT_EQ(limit.load_limit, std::nullopt) << i;
  }

  // Extra load
  for (size_t i = 0; i < 100; i++) {
    congestion_control::v2::Sensor::Data data;
    data.timings_avg_ms = 501;
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

USERVER_NAMESPACE_END
