#include <userver/utils/lazy_shared_ptr.hpp>

#include <vector>

#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

class Cache final {
 public:
  using DataType = std::vector<int>;

  explicit Cache(DataType vector)
      : ptr_(std::make_shared<DataType>(std::move(vector))) {}

  Cache(Cache&&) = delete;

  utils::SharedReadablePtr<DataType> Get() const { return ptr_; }

 private:
  utils::SharedReadablePtr<DataType> ptr_;
};

}  // namespace

UTEST(LazySharedPtr, SimpleLazy) {
  Cache cache({1, 2, 3});
  auto lazy_cache(utils::MakeLazyCachePtr(cache));
  auto ptr = cache.Get();
  ASSERT_EQ(&*lazy_cache, &*ptr);
  ASSERT_EQ(*lazy_cache, std::vector<int>({1, 2, 3}));
}

UTEST(LazySharedPtr, SimpleNonLazy) {
  auto ptr = std::make_shared<std::vector<int>>(std::vector<int>{1, 2, 3});
  utils::LazySharedPtr<std::vector<int>> lazy_cache(ptr);
  ASSERT_EQ(&*lazy_cache, &*ptr);
  ASSERT_EQ(*lazy_cache, std::vector<int>({1, 2, 3}));
}

UTEST(LazySharedPtr, GetShared) {
  Cache cache({1, 2, 3});
  auto lazy_cache(utils::MakeLazyCachePtr(cache));
  const auto& res = lazy_cache.GetShared();
  auto ptr = cache.Get();
  ASSERT_EQ(&*res, &*ptr);
  ASSERT_EQ(*res, std::vector<int>({1, 2, 3}));
}

UTEST(LazySharedPtr, Cast) {
  Cache cache(std::vector<int>{1, 2, 3});
  auto lazy_cache(utils::MakeLazyCachePtr(cache));
  const auto expected_result = cache.Get();
  ASSERT_TRUE(static_cast<bool>(lazy_cache));
  auto ptr = static_cast<std::shared_ptr<const std::vector<int>>>(lazy_cache);
  ASSERT_EQ(&*ptr, &*expected_result);
}

UTEST(LazySharedPtr, CopyMoveLazy) {
  Cache cache(std::vector<int>{1, 2, 3});
  auto lazy_cache_origin(utils::MakeLazyCachePtr(cache));
  utils::LazySharedPtr<std::vector<int>> lazy_cache_copy(lazy_cache_origin);

  ASSERT_EQ(&*lazy_cache_copy, &*lazy_cache_origin);
  ASSERT_EQ(*lazy_cache_copy, std::vector<int>({1, 2, 3}));
  utils::LazySharedPtr<std::vector<int>> lazy_cache_move(
      std::move(lazy_cache_origin));
  auto ptr = cache.Get();
  ASSERT_EQ(&*lazy_cache_move, &*ptr);
  ASSERT_EQ(*lazy_cache_move, std::vector<int>({1, 2, 3}));
}

UTEST(LazySharedPtr, CopyMoveNonLazy) {
  auto ptr = std::make_shared<std::vector<int>>(std::vector<int>{1, 2, 3});
  utils::LazySharedPtr<std::vector<int>> lazy_cache_origin(ptr);
  utils::LazySharedPtr<std::vector<int>> lazy_cache_copy(lazy_cache_origin);
  ASSERT_EQ(&*lazy_cache_copy, &*lazy_cache_origin);

  utils::LazySharedPtr<std::vector<int>> lazy_cache_move(
      std::move(lazy_cache_origin));
  ASSERT_EQ(*lazy_cache_move, std::vector<int>({1, 2, 3}));
}

UTEST(LazySharedPtr, AssignmentLazy) {
  Cache cache({1, 2, 3});
  auto cache_origin(utils::MakeLazyCachePtr(cache));
  auto ptr = cache.Get();
  auto cache_copy = cache_origin;
  ASSERT_EQ(&*cache_origin, &*ptr);
  ASSERT_EQ(&*cache_copy, &*ptr);

  auto cache_move = std::move(cache_origin);
  ASSERT_EQ(&*cache_move, &*ptr);
}

USERVER_NAMESPACE_END
