#include <logging/stacktrace_cache.hpp>

#include <unordered_map>

#include <boost/functional/hash.hpp>
#include <boost/stacktrace.hpp>

#include <cache/lru_map.hpp>
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
const std::string_view kStartOfCoroutine1 =
    "void utils::impl::WrappedCallImpl<";
// release clang
const std::string_view kStartOfCoroutine2 = "utils::impl::WrappedCallImpl<";

}  // namespace

std::string to_string(boost::stacktrace::frame frame) {
  thread_local cache::LruMap<boost::stacktrace::frame, std::string>
      frame_name_cache(10000);
  auto* ptr = frame_name_cache.Get(frame);
  if (ptr) return *ptr;

  auto name = boost::stacktrace::to_string(frame);
  frame_name_cache.Put(frame, name);
  return name;
}

std::string to_string(const boost::stacktrace::stacktrace& st) {
  std::string res;
  res.reserve(200 * st.size());

  size_t i = 0;
  for (const auto frame : st) {
    if (i < 10) {
      res += ' ';
    }
    res += std::to_string(i);
    res += '#';
    res += ' ';
    auto frame_name = stacktrace_cache::to_string(frame);

    if (utils::text::StartsWith(frame_name, kStartOfCoroutine1) ||
        utils::text::StartsWith(frame_name, kStartOfCoroutine2)) {
      /* The rest is a long common stacktrace for a task w/ std::function,
       * WrappedCall, fiber, etc. Almost useless for service debugging.
       */
      res += "[start of coroutine]\n";
      break;
    }

    res += frame_name;
    res += '\n';
    i++;
  }

  return res;
}

}  // namespace logging::stacktrace_cache
