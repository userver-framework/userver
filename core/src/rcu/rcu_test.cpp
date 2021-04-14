#include <utest/utest.hpp>

#include <atomic>

#include <engine/sleep.hpp>
#include <engine/standalone.hpp>
#include <engine/task/task_with_result.hpp>
#include <rcu/rcu.hpp>

using X = std::pair<int, int>;

TEST(Rcu, Ctr) {
  RunInCoro([] { rcu::Variable<X> ptr; });
}

TEST(Rcu, ReadInit) {
  RunInCoro([] {
    rcu::Variable<X> ptr(1, 2);

    auto reader = ptr.Read();
    EXPECT_EQ(std::make_pair(1, 2), *reader);
  });
}

TEST(Rcu, ChangeRead) {
  RunInCoro([] {
    rcu::Variable<X> ptr(1, 2);

    {
      auto writer = ptr.StartWrite();
      writer->first = 3;
      writer.Commit();
    }

    auto reader = ptr.Read();
    EXPECT_EQ(std::make_pair(3, 2), *reader);
  });
}

TEST(Rcu, ChangeCancelRead) {
  RunInCoro([] {
    rcu::Variable<X> ptr(1, 2);

    {
      auto writer = ptr.StartWrite();
      writer->first = 3;
    }

    auto reader = ptr.Read();
    EXPECT_EQ(std::make_pair(1, 2), *reader);
  });
}

TEST(Rcu, AssignRead) {
  RunInCoro([] {
    rcu::Variable<X> ptr(1, 2);

    ptr.Assign({3, 4});

    auto reader = ptr.Read();
    EXPECT_EQ(std::make_pair(3, 4), *reader);
  });
}

TEST(Rcu, AssignFromUniquePtr) {
  RunInCoro([] {
    rcu::Variable<X> ptr(1, 2);
    auto new_value = std::make_unique<X>(3, 4);

    ptr.AssignPtr(std::move(new_value));

    auto reader = ptr.Read();
    EXPECT_EQ(std::make_pair(3, 4), *reader);
  });
}

TEST(Rcu, ReadNotCommitted) {
  RunInCoro([] {
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
  });
}

TEST(Rcu, ReadCommitted) {
  RunInCoro([] {
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
  });
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

TEST(Rcu, Lifetime) {
  using Counted = Counted<struct LifetimeTag>;
  RunInCoro([] {
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
  });
}

TEST(Rcu, ReadablePtrMoveAssign) {
  using Counted = Counted<struct MoveAssignTag>;
  RunInCoro([] {
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
    // Cleanup is done only on writing
    // so we initiate another write operation
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
  });
}

TEST(Rcu, NoCopy) {
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

  RunInCoro([] {
    rcu::Variable<X> ptr(X{1, false});

    // Write
    ptr.Assign(X{2, true});

    // Won't compile if uncomment, no X::X(const X&)
    // auto write_ptr = ptr.StartWrite();

    auto reader = ptr.Read();
    EXPECT_EQ((X{2, true}), *reader);
  });
}

TEST(Rcu, HpCacheReuse) {
  RunInCoro([] {
    std::optional<rcu::Variable<int>> vars;
    vars.emplace(42);
    EXPECT_EQ(42, vars->ReadCopy());

    vars.reset();

    // caused UAF because of stale HP cache -- TAXICOMMON-1506
    vars.emplace(666);
    EXPECT_EQ(666, vars->ReadCopy());
  });
}

TEST(Rcu, AsyncGc) {
  RunInCoro([] {
    class X {
     public:
      X() : task_context_(engine::current_task::GetCurrentTaskContext()) {}

      X(X&& other) : task_context_(other.task_context_) {
        other.task_context_ = nullptr;
      }

      X& operator=(const X&) = delete;

      ~X() {
        EXPECT_NE(engine::current_task::GetCurrentTaskContext(), task_context_);
      }

     private:
      engine::impl::TaskContext* task_context_;
    };

    rcu::Variable<std::unique_ptr<X>> var{std::make_unique<X>()};

    {
      auto read_ptr = var.Read();
      var.Assign(std::make_unique<X>());
    }
    var.Assign(std::unique_ptr<X>());

    engine::Yield();
    engine::Yield();
    engine::Yield();
  });
}

TEST(Rcu, Cleanup) {
  RunInCoro([] {
    static std::atomic<size_t> count{0};
    struct X {
      X() { count++; }
      X(X&&) { count++; }
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
  });
}

TEST(Rcu, ParallelCleanup) {
  RunInCoro([] {
    rcu::Variable<int> ptr(1);

    {
      auto writer = ptr.StartWrite();

      // should not freeze
      ptr.Cleanup();

      writer.Commit();
    }
  });
}

TEST(Rcu, SharedReadablePtr) {
  constexpr int kThreads = 4;
  auto run = [](auto func) { RunInCoro(func, kThreads); };

  run([] {
    constexpr int kIterations = 1000;

    rcu::Variable<int> ptr(0);
    const auto shared_reader = ptr.ReadShared();

    std::atomic<bool> keep_running{true};

    std::vector<engine::TaskWithResult<void>> tasks;
    tasks.reserve(kThreads - 1);

    for (int i = 0; i < kThreads - 1; ++i) {
      tasks.push_back(engine::impl::Async([&] {
        auto reader = ptr.ReadShared();

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
  });
}
