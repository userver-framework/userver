#include <userver/utest/utest.hpp>

#include <atomic>

#include <userver/engine/sleep.hpp>
#include <userver/engine/task/task_with_result.hpp>
#include <userver/rcu/rcu.hpp>
#include <userver/utils/scope_guard.hpp>

#include <engine/task/task_context.hpp>

USERVER_NAMESPACE_BEGIN

using X = std::pair<int, int>;

UTEST(Rcu, Ctr) { rcu::Variable<X> ptr; }

UTEST(Rcu, ReadInit) {
  rcu::Variable<X> ptr(1, 2);

  auto reader = ptr.Read();
  EXPECT_EQ(std::make_pair(1, 2), *reader);
}

UTEST(Rcu, ChangeRead) {
  rcu::Variable<X> ptr(1, 2);

  {
    auto writer = ptr.StartWrite();
    writer->first = 3;
    writer.Commit();
  }

  auto reader = ptr.Read();
  EXPECT_EQ(std::make_pair(3, 2), *reader);
}

UTEST(Rcu, ChangeCancelRead) {
  rcu::Variable<X> ptr(1, 2);

  {
    auto writer = ptr.StartWrite();
    writer->first = 3;
  }

  auto reader = ptr.Read();
  EXPECT_EQ(std::make_pair(1, 2), *reader);
}

UTEST(Rcu, AssignRead) {
  rcu::Variable<X> ptr(1, 2);

  ptr.Assign({3, 4});

  auto reader = ptr.Read();
  EXPECT_EQ(std::make_pair(3, 4), *reader);
}

UTEST(Rcu, ReadNotCommitted) {
  rcu::Variable<X> ptr(1, 2);

  auto reader1 = ptr.Read();
  EXPECT_EQ(std::make_pair(1, 2), *reader1);

  {
    auto writer = ptr.StartWrite();
    writer->second = 3;
    EXPECT_EQ(std::make_pair(1, 2), *reader1);

    auto reader2 = ptr.Read();
    EXPECT_EQ(std::make_pair(1, 2), *reader2);
  }

  EXPECT_EQ(std::make_pair(1, 2), *reader1);

  auto reader3 = ptr.Read();
  EXPECT_EQ(std::make_pair(1, 2), *reader3);
}

UTEST(Rcu, ReadCommitted) {
  rcu::Variable<X> ptr(1, 2);

  auto reader1 = ptr.Read();

  auto writer = ptr.StartWrite();
  writer->second = 3;
  auto reader2 = ptr.Read();

  writer.Commit();
  EXPECT_EQ(std::make_pair(1, 2), *reader1);
  EXPECT_EQ(std::make_pair(1, 2), *reader2);

  auto reader3 = ptr.Read();
  EXPECT_EQ(std::make_pair(1, 3), *reader3);
}

template <typename Tag>
struct Counted {
  Counted() { counter++; }
  Counted(const Counted&) : Counted() {}
  Counted(Counted&&) = delete;

  ~Counted() { counter--; }

  int value{1};

  static inline size_t counter = 0;
};

UTEST(Rcu, Lifetime) {
  using Counted = Counted<struct LifetimeTag>;

  EXPECT_EQ(0, Counted::counter);

  rcu::Variable<Counted> ptr;
  EXPECT_EQ(1, Counted::counter);

  {
    auto reader = ptr.Read();
    EXPECT_EQ(1, Counted::counter);
  }
  EXPECT_EQ(1, Counted::counter);

  {
    auto writer = ptr.StartWrite();
    EXPECT_EQ(2, Counted::counter);
  }
  EXPECT_EQ(1, Counted::counter);

  {
    auto reader2 = ptr.Read();
    EXPECT_EQ(1, Counted::counter);
    {
      auto writer = ptr.StartWrite();
      EXPECT_EQ(2, Counted::counter);

      writer->value = 10;
      writer.Commit();
      EXPECT_EQ(2, Counted::counter);
    }
    EXPECT_EQ(2, Counted::counter);
  }

  EXPECT_EQ(2, Counted::counter);

  {
    auto writer = ptr.StartWrite();
    EXPECT_EQ(3, Counted::counter);

    writer->value = 10;
    writer.Commit();
    engine::Yield();
    EXPECT_EQ(1, Counted::counter);
  }
  EXPECT_EQ(1, Counted::counter);
}

UTEST(Rcu, ReadablePtrMoveAssign) {
  using Counted = Counted<struct MoveAssignTag>;

  EXPECT_EQ(0, Counted::counter);

  rcu::Variable<Counted> ptr;
  EXPECT_EQ(1, Counted::counter);

  auto reader1 = ptr.Read();
  EXPECT_EQ(1, Counted::counter);

  {
    auto writer = ptr.StartWrite();
    writer->value = 10;
    writer.Commit();
    EXPECT_EQ(2, Counted::counter);
  }

  EXPECT_EQ(2, Counted::counter);
  auto reader2 = ptr.Read();
  EXPECT_EQ(2, Counted::counter);

  {
    auto writer = ptr.StartWrite();
    writer->value = 15;
    writer.Commit();
    EXPECT_EQ(3, Counted::counter);
  }

  // state now:
  // reader1 -> value 0
  // reader2 -> value 10
  // main    -> value 15

  reader1 = ptr.Read();
  reader2 = ptr.Read();
  // Cleanup is only done on writing, so we initiate another write operation
  {
    auto writer = ptr.StartWrite();
    writer->value = 20;
    writer.Commit();
    engine::Yield();
  }

  // Expected state:
  // reader1 -> value 15
  // reader2 -> value 15
  // main    -> value 20
  EXPECT_EQ(2, Counted::counter);
  EXPECT_EQ(15, reader1->value);
  EXPECT_EQ(15, reader2->value);
}

UTEST(Rcu, NoCopy) {
  struct X {
    X(int x_, bool y_) : x(x_), y(y_) {}

    X(X&&) = default;
    X(const X&) = delete;
    X& operator=(const X&) = delete;

    bool operator==(const X& other) const {
      return std::tie(x, y) == std::tie(other.x, other.y);
    }

    int x{};
    bool y{};
  };

  rcu::Variable<X> ptr(X{1, false});

  // Write
  ptr.Assign(X{2, true});

  // Won't compile if uncomment, no X::X(const X&)
  // auto write_ptr = ptr.StartWrite();

  auto reader = ptr.Read();
  EXPECT_EQ((X{2, true}), *reader);
}

UTEST(Rcu, HpCacheReuse) {
  std::optional<rcu::Variable<int>> vars;
  vars.emplace(42);
  EXPECT_EQ(42, vars->ReadCopy());

  vars.reset();

  // caused UAF because of stale HP cache -- TAXICOMMON-1506
  vars.emplace(666);
  EXPECT_EQ(666, vars->ReadCopy());
}

UTEST(Rcu, AsyncGc) {
  auto& mutation_task = engine::current_task::GetCurrentTaskContext();

  rcu::Variable<utils::ScopeGuard> var(
      [&] { EXPECT_FALSE(mutation_task.IsCurrent()); });

  {
    auto read_ptr = var.Read();
    var.Emplace([&] { EXPECT_FALSE(mutation_task.IsCurrent()); });
  }

  // destruction of the last value is executed synchronously
  var.Emplace([&] { EXPECT_TRUE(mutation_task.IsCurrent()); });
}

UTEST(Rcu, Cleanup) {
  static std::atomic<size_t> count{0};
  struct X {
    X() { count++; }
    X(X&&) noexcept { count++; }
    X(const X&) { count++; }
    ~X() { count--; }
  };

  rcu::Variable<X> ptr;

  std::optional<rcu::ReadablePtr<X>> r;
  EXPECT_EQ(count.load(), 1);

  {
    auto writer = ptr.StartWrite();
    r.emplace(ptr.Read());
    EXPECT_EQ(count.load(), 2);

    writer.Commit();
  }
  EXPECT_EQ(count.load(), 2);

  r.reset();
  EXPECT_EQ(count.load(), 2);

  ptr.Cleanup();

  // For background delete
  engine::Yield();
  engine::Yield();

  EXPECT_EQ(count.load(), 1);
}

UTEST(Rcu, ParallelCleanup) {
  rcu::Variable<int> ptr(1);

  {
    auto writer = ptr.StartWrite();

    // should not freeze
    ptr.Cleanup();

    writer.Commit();
  }
}

UTEST_MT(Rcu, CopyReadablePtr, 4) {
  constexpr int kThreads = 4;

  constexpr int kIterations = 1000;

  rcu::Variable<int> ptr(0);
  const auto shared_reader = ptr.Read();

  std::atomic<bool> keep_running{true};

  std::vector<engine::TaskWithResult<void>> tasks;
  tasks.reserve(kThreads - 1);

  for (int i = 0; i < kThreads - 1; ++i) {
    tasks.push_back(engine::AsyncNoSpan([&] {
      auto reader = ptr.Read();

      while (keep_running) {
        // replace the original with a copy
        auto reader_copy = reader;
        reader = std::move(reader_copy);

        EXPECT_GE(*reader, 0);
        EXPECT_LT(*reader, kIterations);
      }
    }));
  }

  for (int i = 0; i < kIterations; ++i) {
    ptr.Assign(i);
  }

  keep_running = false;

  for (auto& task : tasks) {
    task.Get();
  }
}

UTEST(Rcu, SampleRcuVariable) {
  /// [Sample rcu::Variable usage]

  constexpr int kOldValue = 1;
  constexpr auto kOldString = "Old string";
  constexpr int kNewValue = 2;
  constexpr auto kNewString = "New string";

  struct Data {
    int x;
    std::string s;
  };
  rcu::Variable<Data> data = Data{kOldValue, kOldString};

  {
    auto ro_ptr = data.Read();
    // We can use ro_ptr to access data.
    // At this time, neither the writer nor the other readers are not blocked
    // => you can hold a smart pointer without fear of blocking other users
    ASSERT_EQ(ro_ptr->x, kOldValue);
    ASSERT_EQ(ro_ptr->s, kOldString);
  }

  {
    auto ptr = data.StartWrite();
    // ptr stores a copy of the latest version of the data, now we can prepare
    // a new version Readers continue to access the old version of the data
    // via .Read()
    ptr->x = kNewValue;
    ptr->s = kNewString;
    // After Commit(), the next reader in Read() gets the version of the data
    // we just wrote. Old readers who did Read() before Commit() continue to
    // work with the old version of the data.
    ptr.Commit();
  }
  /// [Sample rcu::Variable usage]
}

namespace {

struct CleaningUpInt final {
  std::uint64_t value;

  explicit CleaningUpInt(std::uint64_t value) : value(value) {}
  ~CleaningUpInt() { value = 0; }
};

constexpr std::size_t kReadablePtrPingPongTasks = 3;
constexpr std::size_t kReadingTasks = 2;
constexpr std::size_t kWritingTasks = 2;
constexpr std::size_t kSleeperTask = 1;
constexpr std::size_t kTotalTasks =
    kReadablePtrPingPongTasks + kReadingTasks + kWritingTasks + kSleeperTask;

}  // namespace

UTEST_MT(Rcu, TortureTest, kTotalTasks) {
  rcu::Variable<CleaningUpInt> data{1};
  std::atomic<bool> keep_running{true};

  engine::Mutex ping_pong_mutex;
  rcu::ReadablePtr<CleaningUpInt> ptr = data.Read();

  std::vector<engine::TaskWithResult<void>> tasks;

  for (std::size_t i = 0; i < kReadablePtrPingPongTasks; ++i) {
    tasks.push_back(engine::AsyncNoSpan([&] {
      while (keep_running) {
        std::lock_guard lock(ping_pong_mutex);
        // copy a ptr created by another thread
        ptr = rcu::ReadablePtr{ptr};
        ASSERT_GT(ptr->value, 0);
      }
    }));
  }

  for (std::size_t i = 0; i < kReadingTasks; ++i) {
    tasks.push_back(engine::AsyncNoSpan([&] {
      while (keep_running) {
        const auto local_ptr = data.Read();
        ASSERT_GT(local_ptr->value, 0);
      }
    }));
  }

  for (std::size_t i = 0; i < kWritingTasks; ++i) {
    tasks.push_back(engine::AsyncNoSpan([&] {
      while (keep_running) {
        const auto old = data.Read();
        data.Assign(CleaningUpInt{old->value + 1});
      }
    }));
  }

  engine::SleepFor(std::chrono::milliseconds{100});
  keep_running = false;
}

UTEST(Rcu, WritablePtrUnlocksInCommit) {
  rcu::Variable<int> var{1};

  auto writer1 = var.StartWrite();
  ++*writer1;
  writer1.Commit();

  auto writer2 = var.StartWrite();  // should not deadlock
  ++*writer2;
  writer2.Commit();

  EXPECT_EQ(var.ReadCopy(), 3);
}

UTEST(Rcu, NonCopyableNonMovable) {
  rcu::Variable<std::atomic<int>> data{10};

  {
    const auto reader = data.Read();
    EXPECT_EQ(reader->load(), 10);
  }

  {
    data.Emplace(20);
    const auto reader = data.Read();
    EXPECT_EQ(reader->load(), 20);
  }
}

namespace {

class DestructionTracker final {
 public:
  explicit DestructionTracker(std::atomic<bool>& destroyed)
      : destroyed_(destroyed) {}

  ~DestructionTracker() { destroyed_.store(true); }

 private:
  std::atomic<bool>& destroyed_;
};

}  // namespace

UTEST(Rcu, SyncDestruction) {
  std::atomic<bool> destroyed[3]{false, false, false};
  {
    rcu::Variable<DestructionTracker> var{rcu::DestructionType::kSync,
                                          destroyed[0]};
    EXPECT_FALSE(destroyed[0]);
    EXPECT_FALSE(destroyed[1]);
    EXPECT_FALSE(destroyed[2]);

    {
      auto ptr = var.StartWriteEmplace(destroyed[1]);
      EXPECT_FALSE(destroyed[0]);
      EXPECT_FALSE(destroyed[1]);
      EXPECT_FALSE(destroyed[2]);
      ptr.Commit();
      EXPECT_TRUE(destroyed[0]);
      EXPECT_FALSE(destroyed[1]);
      EXPECT_FALSE(destroyed[2]);
    }

    EXPECT_TRUE(destroyed[0]);
    EXPECT_FALSE(destroyed[1]);
    EXPECT_FALSE(destroyed[2]);
    var.Emplace(destroyed[2]);
    EXPECT_TRUE(destroyed[0]);
    EXPECT_TRUE(destroyed[1]);
    EXPECT_FALSE(destroyed[2]);
  }

  EXPECT_TRUE(destroyed[0]);
  EXPECT_TRUE(destroyed[1]);
  EXPECT_TRUE(destroyed[2]);
}

UTEST_MT(Rcu, Core, 3) {
  const auto deadline =
      engine::Deadline::FromDuration(std::chrono::milliseconds{100});
  std::monostate non_null;

  while (deadline.IsReached()) {
    std::atomic<int> old_value{42};
    std::atomic<std::monostate*> is_old_value_current{&non_null};
    std::atomic<std::monostate*> hazard_pointer{nullptr};

    std::vector<engine::TaskWithResult<void>> tasks;
    tasks.reserve(2);

    // reader task
    tasks.push_back(engine::AsyncNoSpan([&] {
      auto* t_ptr_ = &non_null;
      // mimics storing current_ address into a hazard pointer
      hazard_pointer.store(t_ptr_, std::memory_order_seq_cst);

      // mimics checking if the previously read current_ is still current
      if (t_ptr_ == is_old_value_current.load()) {
        // success - check that "the object is alive"
        EXPECT_EQ(old_value, 42);
      }
    }));

    // writer task
    tasks.push_back(engine::AsyncNoSpan([&] {
      // mimics changing current_
      is_old_value_current.store(nullptr);
      if (hazard_pointer.load() == nullptr) {
        // the hazard pointer contains nullptr - no one is using 'old_value'

        // mimic destroying the old object
        old_value.store(0);
      }
    }));

    for (auto& task : tasks) task.Get();
  }
}

USERVER_NAMESPACE_END
