#include <userver/utils/thread_name.hpp>

#include <pthread.h>

#include <array>
#include <cstddef>
#include <stdexcept>
#include <system_error>

#include <userver/logging/log.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils {

namespace {

#if defined(__linux__)
constexpr std::size_t kMaxThreadNameLen = 15;  // + '\0'
#elif defined(__APPLE__)
constexpr std::size_t kMaxThreadNameLen = 63;  // + '\0'
#else
constexpr std::size_t kMaxThreadNameLen = 15;  // + '\0'
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
  return std::string{buf.data()};
}

void SetCurrentThreadName(std::string_view name) {
  if (name.find('\0') != std::string::npos) {
    throw std::logic_error("Thread name contains null byte");
  }
  const auto truncated_name = name.substr(0, kMaxThreadNameLen);
  if (name.size() != truncated_name.size()) {
    LOG_INFO() << "Thread name '" << name << "' is too long, truncated to '"
               << truncated_name << '\'';
  }

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
  std::array<char, kMaxThreadNameLen + 1> buf;
  truncated_name.copy(buf.data(), truncated_name.size());
  buf[truncated_name.size()] = '\0';

  const int ret = ::pthread_setname_np(
// MAC_COMPAT: only supports renaming within the thread, different signature
#ifndef __APPLE__
      ::pthread_self(),
#endif
      buf.data());
  if (ret) {
    throw std::system_error(ret, std::system_category(),
                            "Cannot set thread name");
  }
}

CurrentThreadNameGuard::CurrentThreadNameGuard(std::string_view name)
    : prev_name_(GetCurrentThreadName()) {
  SetCurrentThreadName(name);
}

CurrentThreadNameGuard::~CurrentThreadNameGuard() {
  try {
    SetCurrentThreadName(prev_name_);
  } catch (const std::exception&) {
    // ignore
  }
}

}  // namespace utils

USERVER_NAMESPACE_END
