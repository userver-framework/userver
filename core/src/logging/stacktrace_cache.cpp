#include <logging/stacktrace_cache.hpp>

#include <unordered_map>

#include <boost/functional/hash.hpp>

#include <cache/lru_cache.hpp>

namespace std {

template <>
struct hash<boost::stacktrace::frame> {
  std::size_t operator()(boost::stacktrace::frame frame) const noexcept {
    return boost::hash<boost::stacktrace::frame>()(frame);
  }
};

}  // namespace std

namespace logging {

namespace stacktrace_cache {

std::string to_string(boost::stacktrace::frame frame) {
  thread_local cache::LRU<boost::stacktrace::frame, std::string>
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
    res += stacktrace_cache::to_string(frame);
    res += '\n';
    i++;
  }

  return res;
}

}  // namespace stacktrace_cache
}  // namespace logging
