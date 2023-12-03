#include "time_storage.hpp"

#include <fmt/compile.h>
#include <fmt/format.h>

#include <userver/logging/impl/tag_writer.hpp>
#include <userver/utils/algo.hpp>
#include <userver/utils/fast_scope_guard.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

constexpr std::string_view kTimerSuffix = "_time";
constexpr auto kNsInMs = 1'000'000;

}  // namespace

namespace tracing::impl {

void TimeStorage::PushLap(const std::string& key, Duration value) {
  data_[key] += value;
}

TimeStorage::Duration TimeStorage::DurationTotal(const std::string& key) const {
  return utils::FindOrDefault(data_, key, Duration{0});
}

void TimeStorage::MergeInto(logging::impl::TagWriter writer) {
  fmt::basic_memory_buffer<char, 64> key_buffer{};
  fmt::basic_memory_buffer<char, 16> duration_memory_buffer{};

  for (const auto& [key, value] : data_) {
    const utils::FastScopeGuard buffer_cleanup_scope{
        [&key_buffer, &duration_memory_buffer]() noexcept {
          key_buffer.clear();
          duration_memory_buffer.clear();
        }};

    key_buffer.append(key);
    key_buffer.append(kTimerSuffix);

    const auto ns_count = value.count();
    fmt::format_to(std::back_inserter(duration_memory_buffer),
                   FMT_COMPILE("{}.{:0>6}"), ns_count / kNsInMs,
                   ns_count % kNsInMs);

    const std::string_view key_tag_sw{key_buffer.data(), key_buffer.size()};
    const std::string_view duration_sw{duration_memory_buffer.data(),
                                       duration_memory_buffer.size()};
    writer.PutTag(logging::impl::RuntimeTagKey{key_tag_sw}, duration_sw);
  }
}

}  // namespace tracing::impl

USERVER_NAMESPACE_END
