#include <userver/concurrent/conflated_event_channel.hpp>

#include <userver/engine/mutex.hpp>
#include <userver/engine/semaphore.hpp>

#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

struct Subscriber final {
  Subscriber() : sem(1) {
    sem.lock_shared();  // To set semaphore 0 at start
  }

  void OnEvent() {
    sem.unlock_shared();
    std::lock_guard<engine::Mutex> lock(mutex);
    times_called++;
  }

  ~Subscriber() {
    sem.unlock_shared();  // Return semaphore to 1
  }

  int times_called = 0;
  engine::Mutex mutex;
  engine::Semaphore sem;
};

}  // namespace

UTEST(ConflatedEventChannel, Publish) {
  concurrent::AsyncEventChannel channel("channel");
  concurrent::ConflatedEventChannel conflated_channel("conflated_channel");
  conflated_channel.AddChannel(channel);
  Subscriber subscriber;

  auto subscription =
      conflated_channel.AddListener(&subscriber, "sub", &Subscriber::OnEvent);
  EXPECT_EQ(subscriber.times_called, 0);

  channel.SendEvent();

  subscriber.sem.lock_shared();
  EXPECT_EQ(subscriber.times_called, 1);
}

UTEST(ConflatedEventChannel, PublishMany) {
  concurrent::AsyncEventChannel channel("channel");
  concurrent::ConflatedEventChannel conflated_channel("conflated_channel");
  conflated_channel.AddChannel(channel);

  Subscriber subscriber;

  auto subscription =
      conflated_channel.AddListener(&subscriber, "sub", &Subscriber::OnEvent);
  EXPECT_EQ(subscriber.times_called, 0);

  {
    std::lock_guard<engine::Mutex> lock(subscriber.mutex);

    // E1 calls 'OnEvent' asynchronously, which increments 'sem' and starts
    // waiting for 'mutex'. If 'SendEvent' was synchronous, a deadlock would
    // occur here.
    channel.SendEvent();

    // Wait for E1 to be received. When called rapidly, E2, E3 and E4 could
    // all be conflated with E1, and we don't want it in this test.
    subscriber.sem.lock_shared();
    channel.SendEvent();  // E2
    channel.SendEvent();  // E3 pushes out E2
    channel.SendEvent();  // E4 pushes out E3

    // 'mutex' is unlocked, E1 completes, then E4 is processed
  }

  subscriber.sem.lock_shared();  // unlocked by E4
  EXPECT_EQ(subscriber.times_called, 2);

  // Same as above, but events are sent manually
  {
    std::lock_guard<engine::Mutex> lock(subscriber.mutex);
    conflated_channel.SendEvent();
    subscriber.sem.lock_shared();
    conflated_channel.SendEvent();
    conflated_channel.SendEvent();
    conflated_channel.SendEvent();
  }
  subscriber.sem.lock_shared();
  EXPECT_EQ(subscriber.times_called, 4);
}

UTEST(ConflatedEventChannel, PublishFromManyChannels) {
  concurrent::AsyncEventChannel channel1("channel1");
  concurrent::AsyncEventChannel channel2("channel2");
  concurrent::ConflatedEventChannel conflated_channel("conflated_channel");
  conflated_channel.AddChannel(channel1);
  conflated_channel.AddChannel(channel2);

  Subscriber subscriber;

  auto subscription =
      conflated_channel.AddListener(&subscriber, "sub", &Subscriber::OnEvent);
  EXPECT_EQ(subscriber.times_called, 0);

  channel1.SendEvent();
  subscriber.sem.lock_shared();
  EXPECT_EQ(subscriber.times_called, 1);

  channel2.SendEvent();
  subscriber.sem.lock_shared();
  EXPECT_EQ(subscriber.times_called, 2);

  {
    std::lock_guard<engine::Mutex> lock(subscriber.mutex);
    channel1.SendEvent();
    subscriber.sem.lock_shared();
    channel2.SendEvent();
    channel1.SendEvent();
    channel2.SendEvent();
  }
  subscriber.sem.lock_shared();
  EXPECT_EQ(subscriber.times_called, 4);
}

UTEST(ConflatedEventChannel, PublishForManyListeners) {
  concurrent::AsyncEventChannel channel("channel");
  concurrent::ConflatedEventChannel conflated_channel("conflated_channel");
  conflated_channel.AddChannel(channel);

  Subscriber subscriber1;
  Subscriber subscriber2;

  auto subscription1 =
      conflated_channel.AddListener(&subscriber1, "sub1", &Subscriber::OnEvent);
  auto subscription2 =
      conflated_channel.AddListener(&subscriber2, "sub2", &Subscriber::OnEvent);
  EXPECT_EQ(subscriber1.times_called, 0);
  EXPECT_EQ(subscriber2.times_called, 0);

  channel.SendEvent();

  subscriber1.sem.lock_shared();
  subscriber2.sem.lock_shared();
  EXPECT_EQ(subscriber1.times_called, 1);
  EXPECT_EQ(subscriber2.times_called, 1);
}

UTEST(ConflatedEventChannel, UpdateAndListen) {
  concurrent::AsyncEventChannel channel("channel");
  concurrent::ConflatedEventChannel conflated_channel("conflated_channel");
  conflated_channel.AddChannel(channel);

  Subscriber subscriber;

  auto subscription = conflated_channel.UpdateAndListen(&subscriber, "sub",
                                                        &Subscriber::OnEvent);
  EXPECT_EQ(subscriber.times_called, 1);

  channel.SendEvent();
  subscriber.sem.lock_shared();
  subscriber.sem.lock_shared();
  EXPECT_EQ(subscriber.times_called, 2);
}

USERVER_NAMESPACE_END
