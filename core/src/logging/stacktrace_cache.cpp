#include <logging/stacktrace_cache.hpp>

#include <unordered_map>

#include <fmt/format.h>
#include <boost/functional/hash.hpp>
#include <boost/stacktrace.hpp>

#include <cache/lru_map.hpp>
#include <utils/assert.hpp>
#include <utils/text.hpp>

namespace std {

template <>
struct hash<boost::stacktrace::frame> {
  std::size_t operator()(boost::stacktrace::frame frame) const noexcept {
    return boost::hash<boost::stacktrace::frame>()(frame);
  }
};

}  // namespace std

namespace logging::stacktrace_cache {

namespace {

// debug clang
constexpr std::string_view kStartOfCoroutinePrefix1 =
    "void utils::impl::WrappedCallImpl<";
// release clang
constexpr std::string_view kStartOfCoroutinePrefix2 =
    "utils::impl::WrappedCallImpl<";
// fallback for badly stripped names
constexpr std::string_view kStartOfCoroutineSuffix =
    "/utils/wrapped_call.hpp:121";

const std::string& ToStringCachedFiltered(boost::stacktrace::frame frame) {
  thread_local cache::LruMap<boost::stacktrace::frame, std::string>
      frame_name_cache(10000);
  auto* ptr = frame_name_cache.Get(frame);
  if (!ptr) {
    auto name = boost::stacktrace::to_string(frame);
    UASSERT(!name.empty());
    if (utils::text::StartsWith(name, kStartOfCoroutinePrefix1) ||
        utils::text::StartsWith(name, kStartOfCoroutinePrefix2) ||
        utils::text::EndsWith(name, kStartOfCoroutineSuffix)) {
      name = {};
    }
    ptr = frame_name_cache.Emplace(frame, std::move(name));
  }
  return *ptr;
}

}  // namespace

std::string to_string(const boost::stacktrace::stacktrace& st) {
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

}  // namespace logging::stacktrace_cache
