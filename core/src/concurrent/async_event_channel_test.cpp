#include <userver/utest/utest.hpp>

#include <userver/concurrent/async_event_channel.hpp>
#include <userver/engine/sleep.hpp>

USERVER_NAMESPACE_BEGIN

TEST(AsyncEventChannel, Ctr) {
  concurrent::AsyncEventChannel<int> channel("channel");
}

class Subscriber final {
 public:
  Subscriber(int& x) : x_(x) {}

  void OnEvent(int x) { x_ = x; }

 private:
  int& x_;
};

UTEST(AsyncEventChannel, Publish) {
  concurrent::AsyncEventChannel<int> channel("channel");

  int value{0};
  Subscriber s(value);
  EXPECT_EQ(value, 0);

  auto sub = channel.AddListener(&s, "", &Subscriber::OnEvent);
  EXPECT_EQ(value, 0);

  channel.SendEvent(1);
  engine::Yield();
  EXPECT_EQ(value, 1);

  sub.Unsubscribe();
}

UTEST(AsyncEventChannel, Unsubscribe) {
  concurrent::AsyncEventChannel<int> channel("channel");

  int value{0};
  Subscriber s(value);
  auto sub = channel.AddListener(&s, "", &Subscriber::OnEvent);

  channel.SendEvent(1);
  engine::Yield();
  EXPECT_EQ(value, 1);

  sub.Unsubscribe();
  channel.SendEvent(2);
  engine::Yield();
  engine::Yield();
  EXPECT_EQ(value, 1);
}

UTEST(AsyncEventChannel, PublishTwoSubscribers) {
  concurrent::AsyncEventChannel<int> channel("channel");

  int value1{0}, value2{0};
  Subscriber s1(value1), s2(value2);

  auto sub1 = channel.AddListener(&s1, "", &Subscriber::OnEvent);
  auto sub2 = channel.AddListener(&s2, "", &Subscriber::OnEvent);
  EXPECT_EQ(value1, 0);
  EXPECT_EQ(value2, 0);

  channel.SendEvent(1);
  engine::Yield();
  EXPECT_EQ(value1, 1);
  EXPECT_EQ(value2, 1);

  sub1.Unsubscribe();
  sub2.Unsubscribe();
}

UTEST(AsyncEventChannel, PublishException) {
  concurrent::AsyncEventChannel<int> channel("channel");

  struct X {
    void OnEvent(int) { throw std::runtime_error("error msg"); }
  };
  X x;

  auto sub1 = channel.AddListener(&x, "subscriber", &X::OnEvent);
  EXPECT_NO_THROW(channel.SendEvent(1));
  sub1.Unsubscribe();
}

USERVER_NAMESPACE_END
