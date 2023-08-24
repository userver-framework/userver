#pragma once

#include <userver/utils/statistics/entry.hpp>
#include <userver/utils/statistics/storage.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {

struct Entry::Impl final {
  Storage* storage{nullptr};
  impl::StorageIterator iterator{};
};

}  // namespace utils::statistics

USERVER_NAMESPACE_END
