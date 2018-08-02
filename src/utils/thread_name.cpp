#include "thread_name.hpp"

#include <pthread.h>

#include <array>
#include <cassert>
#include <stdexcept>
#include <system_error>

#include <logging/log.hpp>

namespace utils {

namespace {

constexpr size_t kMaxThreadNameLen = 15;  // + '\0'

}  // namespace

std::string GetThreadName(std::thread& thread) {
  std::array<char, kMaxThreadNameLen + 1> buf;

  int ret =
      ::pthread_getname_np(thread.native_handle(), buf.data(), buf.size());
  if (ret) {
    throw std::system_error(ret, std::system_category(),
                            "Cannot get thread name");
  }
  return buf.data();
}

void SetThreadName(std::thread& thread, const std::string& name) {
  if (!thread.joinable()) {
    throw std::runtime_error("Cannot rename a non-joinable thread");
  }
  if (name.empty()) {
    throw std::logic_error("Cannot set empty thread name");
  }
  if (name.find('\0') != std::string::npos) {
    throw std::logic_error("Thread name contains null byte");
  }
  const auto& truncated_name = name.substr(0, kMaxThreadNameLen);
  if (name.size() != truncated_name.size()) {
    assert(!"thread name is too long");
    // should be ERROR according to segoon@ but I insist -- bugaevskiy@
    LOG_WARNING() << "Thread name '" << name << "' is too long, truncated to '"
                  << truncated_name << '\'';
  }
  int ret =
      ::pthread_setname_np(thread.native_handle(), truncated_name.c_str());
  if (ret) {
    throw std::system_error(ret, std::system_category(),
                            "Cannot set thread name");
  }
}

}  // namespace utils
