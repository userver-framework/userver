#include <cache/dump/meta_containers.hpp>

#include <utest/utest.hpp>

TEST(CacheDumpMetaContainers, IsReservable) {
  struct ReservableDummy {
    void reserve(std::size_t) {}
  };

  struct NonReservableDummy {};

  static_assert(cache::dump::kIsReservable<std::vector<int>>);
  static_assert(cache::dump::kIsReservable<std::string>);
  static_assert(cache::dump::kIsReservable<ReservableDummy>);

  static_assert(!cache::dump::kIsReservable<std::set<int>>);
  static_assert(!cache::dump::kIsReservable<int>);
  static_assert(!cache::dump::kIsReservable<NonReservableDummy>);
}
