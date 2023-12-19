#include <utils/jemalloc.hpp>

#ifdef JEMALLOC_ENABLED
#include <jemalloc/jemalloc.h>
#else
#include <cerrno>
#endif

#include <userver/utils/thread_name.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::jemalloc {

namespace {

#ifndef JEMALLOC_ENABLED
int mallctl(const char*, void*, size_t*, void*, size_t) { return ENOTSUP; }

void malloc_stats_print(void (*write_cb)(void*, const char*), void* je_cbopaque,
                        const char*) {
  write_cb(je_cbopaque, "(libjemalloc support is disabled)");
}
#endif

std::error_code MakeErrorCode(int rc) { return {rc, std::system_category()}; }

template <typename T>
std::error_code MallCtl(const char* name, T new_value) {
  int rc = mallctl(name, nullptr, nullptr, &new_value, sizeof(new_value));
  return MakeErrorCode(rc);
}

std::error_code MallCtl(const char* name) {
  int rc = mallctl(name, nullptr, nullptr, nullptr, 0);
  return MakeErrorCode(rc);
}

void MallocStatPrintCb(void* data, const char* msg) {
  auto* s = static_cast<std::string*>(data);
  *s += msg;
}

}  // namespace

std::string Stats() {
  std::string result;
  malloc_stats_print(&MallocStatPrintCb, &result, nullptr);
  return result;
}

std::error_code ProfActivate() { return MallCtl<bool>("prof.active", true); }

std::error_code ProfDeactivate() { return MallCtl<bool>("prof.active", false); }

std::error_code ProfDump() { return MallCtl("prof.dump"); }

std::error_code SetMaxBgThreads(size_t max_bg_threads) {
  return MallCtl<size_t>("max_background_threads", max_bg_threads);
}

std::error_code EnableBgThreads() {
  utils::CurrentThreadNameGuard bg_thread_name_guard("je_bg_thread");
  return MallCtl<bool>("background_thread", true);
}

std::error_code StopBgThreads() {
  return MallCtl<bool>("background_thread", false);
}

}  // namespace utils::jemalloc

USERVER_NAMESPACE_END
