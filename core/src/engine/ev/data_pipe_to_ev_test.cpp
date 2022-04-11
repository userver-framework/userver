#include <engine/ev/data_pipe_to_ev.hpp>

#include <thread>

#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

UTEST(DataPipeToEv, Basic) {
  using namespace engine::ev::impl;
  DoubleBufferingState state;

  DoubleBufferingState::ProducerLock{state};
  EXPECT_TRUE(engine::ev::impl::DoubleBufferingState::ConsumerLock{state});
  EXPECT_FALSE(engine::ev::impl::DoubleBufferingState::ConsumerLock{state});
}

UTEST(DataPipeToEv, Nonblocking) {
  using namespace engine::ev::impl;
  DoubleBufferingState state;

  {
    DoubleBufferingState::ProducerLock producer{state};
    EXPECT_FALSE(engine::ev::impl::DoubleBufferingState::ConsumerLock{state});
  }
  EXPECT_TRUE(engine::ev::impl::DoubleBufferingState::ConsumerLock{state});
}

UTEST(DataPipeToEv, Overwriting) {
  using namespace engine::ev::impl;
  DoubleBufferingState state;

  DoubleBufferingState::ProducerLock{state};
  DoubleBufferingState::ProducerLock{state};
  EXPECT_TRUE(engine::ev::impl::DoubleBufferingState::ConsumerLock{state});
  EXPECT_FALSE(engine::ev::impl::DoubleBufferingState::ConsumerLock{state});
}

UTEST(DataPipeToEv, ExpectLastData) {
  engine::ev::DataPipeToEv<int> pipe;

  pipe.Push(1);
  pipe.Push(2);
  auto data = pipe.TryPop();
  ASSERT_TRUE(data);
  EXPECT_EQ(*data, 2);

  pipe.Push(3);
  data = pipe.TryPop();
  ASSERT_TRUE(data);
  EXPECT_EQ(*data, 3);

  pipe.Push(4);
  pipe.Push(5);
  pipe.Push(6);
  data = pipe.TryPop();
  ASSERT_TRUE(data);
  EXPECT_EQ(*data, 6);
}

UTEST(DataPipeToEv, PopIncreasingData) {
  using TestDataPipe = engine::ev::DataPipeToEv<int>;
  TestDataPipe pipe;

  constexpr int kUpdatesCount = 10000;

  std::thread ev_loop([&pipe, max_value = kUpdatesCount] {
    int previous_value = 0;

    while (previous_value != max_value) {
      auto param = pipe.TryPop();
      if (!param) {
        continue;
      }
      ASSERT_LT(previous_value, *param);
      previous_value = *param;
    }
  });

  for (int i = 1; i <= kUpdatesCount; ++i) {
    pipe.Push(int{i});
  }

  ev_loop.join();
}

USERVER_NAMESPACE_END
