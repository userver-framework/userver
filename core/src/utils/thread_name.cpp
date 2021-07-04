#include "thread_name.hpp"

#include <pthread.h>

#include <array>
#include <stdexcept>
#include <system_error>

#include <userver/logging/log.hpp>
#include <userver/utils/assert.hpp>

namespace utils {

namespace {

#if defined(__linux__)
constexpr size_t kMaxThreadNameLen = 15;  // + '\0'
#elif defined(__APPLE__)
constexpr size_t kMaxThreadNameLen = 63;  // + '\0'
#endif

}  // namespace

std::string GetCurrentThreadName() {
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
  std::array<char, kMaxThreadNameLen + 1> buf;

  int ret = ::pthread_getname_np(::pthread_self(), buf.data(), buf.size());
  if (ret) {
    throw std::system_error(ret, std::system_category(),
                            "Cannot get thread name");
  }
  return buf.data();
}

void SetCurrentThreadName(const std::string& name) {
  if (name.find('\0') != std::string::npos) {
    throw std::logic_error("Thread name contains null byte");
  }
  const auto& truncated_name = name.substr(0, kMaxThreadNameLen);
  if (name.size() != truncated_name.size()) {
    LOG_WARNING() << "Thread name '" << name << "' is too long, truncated to '"
                  << truncated_name << '\'';
  }

  int ret = ::pthread_setname_np(
// MAC_COMPAT: only supports renaming within the thread, different signature
#ifndef __APPLE__
      ::pthread_self(),
#endif
      truncated_name.c_str());
  if (ret) {
    throw std::system_error(ret, std::system_category(),
                            "Cannot set thread name");
  }
}

}  // namespace utils
