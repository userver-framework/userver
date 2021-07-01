#include <string>

#include <utest/utest.hpp>

#include <cache/expirable_lru_cache.hpp>
#include <engine/sleep.hpp>
#include <utils/mock_now.hpp>

namespace {

using SimpleCacheKey = std::string;
using SimpleCacheValue = int;
using SimpleCache = cache::ExpirableLruCache<SimpleCacheKey, SimpleCacheValue>;
using SimpleWrapper = cache::LruCacheWrapper<SimpleCacheKey, SimpleCacheValue>;

void EngineYield() {
  engine::Yield();
  engine::Yield();
}

class Counter {
 public:
  Counter(int value) : value_(value) {}

  Counter() : Counter(0) {}

  void Flush() { value_ = 0; }

  bool operator==(const Counter& other) const { return value_ == other.value_; }

  bool operator!=(const Counter& other) const { return !(*this == other); }

  Counter& operator++() {
    value_++;
    return *this;
  }

  static const Counter& Zero() {
    static const Counter counter(0);
    return counter;
  }

  static const Counter& One() {
    static const Counter counter(1);
    return counter;
  }

 private:
  std::atomic<int> value_;
};

std::function<SimpleCacheValue(SimpleCacheKey)> UpdateNever() {
  return [](SimpleCacheKey) -> SimpleCacheValue {
    ADD_FAILURE() << "Call of 'Update' should never happen";
    return 0;
  };
}

std::function<SimpleCacheValue(SimpleCacheKey)> UpdateValue(
    std::shared_ptr<Counter> counter, SimpleCacheValue value) {
  return [counter_ = std::move(counter),
          value_ = std::move(value)](SimpleCacheKey) {
    ++(*counter_);
    return value_;
  };
}

SimpleCache CreateSimpleCache() { return SimpleCache(1, 1); }

std::shared_ptr<SimpleCache> CreateSimpleCachePtr() {
  return std::make_shared<SimpleCache>(1, 1);
}

}  // namespace

UTEST(ExpirableLruCache, Hit) {
  auto counter = std::make_shared<Counter>();

  auto cache = CreateSimpleCache();
  const SimpleCacheKey key = "my-key";

  counter->Flush();
  EXPECT_EQ(1, cache.Get(key, UpdateValue(counter, 1)));
  EXPECT_EQ(Counter::One(), *counter);

  EXPECT_EQ(1, cache.Get(key, UpdateNever()));
}

UTEST(ExpirableLruCache, HitOptional) {
  auto counter = std::make_shared<Counter>();

  auto cache = CreateSimpleCache();
  SimpleCacheKey key = "my-key";

  EXPECT_EQ(std::nullopt, cache.GetOptional(key, UpdateNever()));

  counter->Flush();
  EXPECT_EQ(1, cache.Get(key, UpdateValue(counter, 1)));
  EXPECT_EQ(Counter::One(), *counter);

  EXPECT_EQ(std::make_optional(1), cache.GetOptional(key, UpdateNever()));
}

UTEST(ExpirableLruCache, HitOptionalUnexpirable) {
  auto counter = std::make_shared<Counter>();

  auto cache = CreateSimpleCache();
  cache.SetMaxLifetime(std::chrono::seconds(2));
  SimpleCacheKey key = "my-key";

  utils::datetime::MockNowSet(std::chrono::system_clock::now());
  counter->Flush();

  cache.Put(key, 1);

  for (int i = 0; i < 10; i++) {
    utils::datetime::MockSleep(std::chrono::seconds(2));
    EXPECT_EQ(1, cache.GetOptionalUnexpirable(key));
  }
}

UTEST(ExpirableLruCache, HitOptionalUnexpirableWithUpdate) {
  auto counter = std::make_shared<Counter>();

  auto cache = CreateSimpleCache();
  cache.SetMaxLifetime(std::chrono::seconds(2));
  cache.SetBackgroundUpdate(cache::BackgroundUpdateMode::kEnabled);
  SimpleCacheKey key = "my-key";

  utils::datetime::MockNowSet(std::chrono::system_clock::now());
  counter->Flush();

  cache.Put(key, 1);
  utils::datetime::MockSleep(std::chrono::seconds(3));
  EXPECT_EQ(
      1, cache.GetOptionalUnexpirableWithUpdate(key, UpdateValue(counter, 2)));

  EngineYield();

  EXPECT_EQ(
      2, cache.GetOptionalUnexpirableWithUpdate(key, UpdateValue(counter, 2)));
}

UTEST(ExpirableLruCache, HitOptionalNoUpdate) {
  auto counter = std::make_shared<Counter>();

  auto cache = CreateSimpleCache();
  cache.SetMaxLifetime(std::chrono::seconds(2));
  SimpleCacheKey key = "my-key";

  utils::datetime::MockNowSet(std::chrono::system_clock::now());
  counter->Flush();

  cache.Put(key, 1);
  EXPECT_EQ(1, cache.GetOptionalNoUpdate(key));
  utils::datetime::MockSleep(std::chrono::seconds(1));

  EXPECT_EQ(1, cache.GetOptionalNoUpdate(key));
  utils::datetime::MockSleep(std::chrono::seconds(1));

  EXPECT_EQ(1, cache.GetOptionalNoUpdate(key));
  utils::datetime::MockSleep(std::chrono::seconds(1));

  EXPECT_EQ(std::nullopt, cache.GetOptionalNoUpdate(key));
}

UTEST(ExpirableLruCache, NoCache) {
  auto counter = std::make_shared<Counter>();

  auto cache = CreateSimpleCache();
  const auto read_mode = SimpleCache::ReadMode::kSkipCache;

  SimpleCacheKey key = "my-key";

  counter->Flush();
  EXPECT_EQ(1, cache.Get(key, UpdateValue(counter, 1), read_mode));
  EXPECT_EQ(Counter::One(), *counter);

  counter->Flush();
  EXPECT_EQ(2, cache.Get(key, UpdateValue(counter, 2), read_mode));
  EXPECT_EQ(Counter::One(), *counter);
}

UTEST(ExpirableLruCache, Expire) {
  auto counter = std::make_shared<Counter>();

  auto cache = CreateSimpleCache();
  cache.SetMaxLifetime(std::chrono::seconds(2));
  SimpleCacheKey key = "my-key";

  utils::datetime::MockNowSet(std::chrono::system_clock::now());

  counter->Flush();
  EXPECT_EQ(1, cache.Get(key, UpdateValue(counter, 1)));
  EXPECT_EQ(Counter::One(), *counter);

  EXPECT_EQ(1, cache.Get(key, UpdateNever()));

  utils::datetime::MockSleep(std::chrono::seconds(3));

  counter->Flush();
  EXPECT_EQ(2, cache.Get(key, UpdateValue(counter, 2)));
  EXPECT_EQ(Counter::One(), *counter);
}

UTEST(ExpirableLruCache, DefaultNoExpire) {
  auto counter = std::make_shared<Counter>();

  auto cache = CreateSimpleCache();
  SimpleCacheKey key = "key";

  utils::datetime::MockNowSet(std::chrono::system_clock::now());

  counter->Flush();
  EXPECT_EQ(1, cache.Get(key, UpdateValue(counter, 1)));
  EXPECT_EQ(Counter::One(), *counter);

  for (int i = 0; i < 10; i++) {
    utils::datetime::MockSleep(std::chrono::seconds(2));
    EXPECT_EQ(1, cache.Get(key, UpdateNever()));
  }
}

UTEST(ExpirableLruCache, InvalidateByKey) {
  auto counter = std::make_shared<Counter>();

  auto cache = CreateSimpleCache();
  SimpleCacheKey key = "my-key";

  counter->Flush();
  EXPECT_EQ(1, cache.Get(key, UpdateValue(counter, 1)));
  EXPECT_EQ(Counter::One(), *counter);

  cache.InvalidateByKey(key);

  counter->Flush();
  EXPECT_EQ(2, cache.Get(key, UpdateValue(counter, 2)));
  EXPECT_EQ(Counter::One(), *counter);
}

UTEST(ExpirableLruCache, BackgroundUpdate) {
  auto counter = std::make_shared<Counter>();

  auto cache = CreateSimpleCache();
  cache.SetMaxLifetime(std::chrono::seconds(3));
  cache.SetBackgroundUpdate(cache::BackgroundUpdateMode::kEnabled);

  SimpleCacheKey key = "my-key";

  utils::datetime::MockNowSet(std::chrono::system_clock::now());

  counter->Flush();
  EXPECT_EQ(1, cache.Get(key, UpdateValue(counter, 1)));
  EXPECT_EQ(Counter::One(), *counter);

  EXPECT_EQ(1, cache.Get(key, UpdateNever()));

  EngineYield();

  EXPECT_EQ(1, cache.Get(key, UpdateNever()));

  utils::datetime::MockSleep(std::chrono::seconds(2));

  counter->Flush();
  EXPECT_EQ(1, cache.Get(key, UpdateValue(counter, 2)));

  EngineYield();

  EXPECT_EQ(Counter::One(), *counter);
  EXPECT_EQ(2, cache.Get(key, UpdateNever()));
}

UTEST(LruCacheWrapper, HitWrapper) {
  auto counter = std::make_shared<Counter>();

  auto cache_ptr = CreateSimpleCachePtr();
  SimpleWrapper wrapper(cache_ptr, UpdateValue(counter, 1));

  SimpleCacheKey key = "my-key";

  counter->Flush();
  EXPECT_EQ(std::nullopt, wrapper.GetOptional(key));
  EXPECT_EQ(Counter::Zero(), *counter);

  counter->Flush();
  EXPECT_EQ(1, wrapper.Get(key));
  EXPECT_EQ(Counter::One(), *counter);

  counter->Flush();
  EXPECT_EQ(std::make_optional(1), wrapper.GetOptional(key));
  EXPECT_EQ(Counter::Zero(), *counter);
}
