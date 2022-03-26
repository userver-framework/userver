#include <userver/logging/stacktrace_cache.hpp>

#include <atomic>
#include <unordered_map>

#include <fmt/format.h>
#include <boost/functional/hash.hpp>
#include <boost/stacktrace.hpp>

#include <userver/cache/lru_map.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/text.hpp>

namespace std {

template <>
struct hash<boost::stacktrace::frame> {
  std::size_t operator()(boost::stacktrace::frame frame) const noexcept {
    return boost::hash<boost::stacktrace::frame>()(frame);
  }
};

}  // namespace std

USERVER_NAMESPACE_BEGIN

namespace logging::stacktrace_cache {

namespace {

constexpr std::string_view kStartOfCoroutine = "utils::impl::WrappedCallImpl<";

std::atomic<bool> stacktrace_enabled{true};

const std::string& ToStringCachedFiltered(boost::stacktrace::frame frame) {
  thread_local cache::LruMap<boost::stacktrace::frame, std::string>
      frame_name_cache(10000);
  auto* ptr = frame_name_cache.Get(frame);
  if (!ptr) {
    auto name = boost::stacktrace::to_string(frame);
    UASSERT(!name.empty());
    if (name.find(kStartOfCoroutine) != std::string::npos) {
      name = {};
    }
    ptr = frame_name_cache.Emplace(frame, std::move(name));
  }
  return *ptr;
}

}  // namespace

std::string to_string(const boost::stacktrace::stacktrace& st) {
  if (!stacktrace_enabled.load()) {
    return "<unknown>";
  }

  std::string res;
  res.reserve(200 * st.size());

  size_t i = 0;
  for (const auto frame : st) {
    if (i < 10) {
      res += ' ';
    }
    res += fmt::to_string(i);
    res += '#';
    res += ' ';
    const auto& filtered_frame_name = ToStringCachedFiltered(frame);

    if (filtered_frame_name.empty()) {
      /* The rest is a long common stacktrace for a task w/ std::function,
       * WrappedCall, fiber, etc. Almost useless for service debugging.
       */
      res += "[start of coroutine]\n";
      break;
    }

    res += filtered_frame_name;
    res += '\n';
    i++;
  }

  return res;
}

bool GlobalEnableStacktrace(bool enable) {
  return stacktrace_enabled.exchange(enable);
}

StacktraceGuard::StacktraceGuard(bool enabled)
    : old_(logging::stacktrace_cache::GlobalEnableStacktrace(enabled)) {}

StacktraceGuard::~StacktraceGuard() {
  logging::stacktrace_cache::GlobalEnableStacktrace(old_);
}

}  // namespace logging::stacktrace_cache

USERVER_NAMESPACE_END
