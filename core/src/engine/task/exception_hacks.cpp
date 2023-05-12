#include <engine/task/exception_hacks.hpp>

#if defined(__has_feature)
#if __has_feature(thread_sanitizer)
#define HAS_TSAN 1
#endif
#endif

// Thread Sanitizer uses dl_iterate_phdr function on initialization and fails if
// we provide our own.
#if !defined(USERVER_DISABLE_PHDR_CACHE) && defined(__linux__) && \
    !defined(HAS_TSAN)
#define USE_PHDR_CACHE 1  // NOLINT
#endif

#ifdef USE_PHDR_CACHE

#include <dlfcn.h>
#include <link.h>

#include <atomic>
#include <vector>

#include <fmt/format.h>

#include <userver/utils/assert.hpp>
#include <userver/utils/impl/userver_experiments.hpp>
#include <utils/impl/assert_extra.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

namespace {

using DlIterateCb = int (*)(struct dl_phdr_info* info, size_t size, void* data);
using DlIterateFn = int (*)(DlIterateCb callback, void* data);

DlIterateFn GetOriginalDlIteratePhdr() {
  static void* func = dlsym(RTLD_NEXT, "dl_iterate_phdr");
  if (!func) {
    utils::impl::AbortWithStacktrace(
        "Cannot find dl_iterate_phdr function with dlsym");
  }

  return reinterpret_cast<DlIterateFn>(func);
}

using PhdrCacheStorage = std::vector<dl_phdr_info>;
std::atomic<PhdrCacheStorage*> phdr_cache_ptr{nullptr};

class PhdrCache final {
 public:
  void Initialize() {
    if (!utils::impl::kPhdrCacheExperiment.IsEnabled()) {
      return;
    }

    GetOriginalDlIteratePhdr()(
        [](dl_phdr_info* info, size_t /* size */, void* data) {
          reinterpret_cast<PhdrCacheStorage*>(data)->push_back(*info);
          return 0;
        },
        &cache_);

    [[maybe_unused]] const auto* old_cache = phdr_cache_ptr.exchange(&cache_);
    UASSERT(!old_cache);
  }

  static void Teardown() { phdr_cache_ptr.store(nullptr); }

  ~PhdrCache() { Teardown(); }

  static bool IsDynamicLoadingEnabled() {
    return phdr_cache_ptr.load() == nullptr;
  }

 private:
  PhdrCacheStorage cache_{};
};
PhdrCache phdr_cache{};

int DlIteratePhdr(DlIterateCb callback, void* data) {
  auto* current_phdr_cache = phdr_cache_ptr.load();

  if (!current_phdr_cache) {
    return GetOriginalDlIteratePhdr()(callback, data);
  }

  int result = 0;
  for (auto& entry : *current_phdr_cache) {
    // Quoting seastar
    // (https://github.com/scylladb/seastar/commit/464f5e3ae43b366b05573018fc46321863bf2fae#diff-c2217f7d679583f3c5d9ce79cdd2facaa46457506d6c65d2b181853c1ee4bd94R97):
    // "Pass dl_phdr_info size that does not include dlpi_adds and dlpi_subs.
    // This forces libgcc to disable caching which is not thread safe and
    // requires dl_iterate_phdr to serialize calls to callback. Since we do
    // not serialize here we have to disable caching."
    //
    // dlpi_adds and dlpi_subs were added in glibc 2.4
    // (https://man7.org/linux/man-pages/man3/dl_iterate_phdr.3.html) and
    // libgcc uses these fields as a notification that cache should be
    // invalidated. We set size to pre-2.4 version to trick libgcc into
    // following: no fields -> no invalidation notification -> no cache.
    result = callback(&entry, offsetof(dl_phdr_info, dlpi_adds), data);
    if (result != 0) {
      break;
    }
  }

  return result;
}

void AssertDynamicLoadingEnabled(std::string_view dl_function_name) {
  if (!PhdrCache::IsDynamicLoadingEnabled()) {
    const auto message = fmt::format(
        "userver forbids '{}' usage during components system lifetime due "
        "to implementation details of making C++ exceptions scalable. You "
        "may disable this optimization by either setting cmake option "
        "USERVER_DISABLE_PHDR_CACHE or moving dynamic libraries "
        "loading/unloading into components constructors/destructors.",
        dl_function_name);
    utils::impl::AbortWithStacktrace(message);
  }
}

void AssertDlFunctionFound(void* function, std::string_view dl_function_name) {
  if (!function) {
    const auto message =
        fmt::format("Cannot find {} function with dlsym", dl_function_name);
    utils::impl::AbortWithStacktrace(message);
  }
}

}  // namespace

void InitPhdrCacheAndDisableDynamicLoading() { phdr_cache.Initialize(); }

void TeardownPhdrCacheAndEnableDynamicLoading() { PhdrCache::Teardown(); }

}  // namespace engine::impl

USERVER_NAMESPACE_END

extern "C" {
#ifndef __clang__
[[gnu::visibility("default")]] [[gnu::externally_visible]]
#endif
    int
    dl_iterate_phdr(USERVER_NAMESPACE::engine::impl::DlIterateCb callback,
                    void* data) {
  return USERVER_NAMESPACE::engine::impl::DlIteratePhdr(callback, data);
}

#ifndef __clang__
[[gnu::visibility("default")]] [[gnu::externally_visible]]
#endif
// NOLINTNEXTLINE(readability-inconsistent-declaration-parameter-name)
void* dlopen(const char* filename, int flags) {
  using DlOpenSignature = void* (*)(const char*, int);
  constexpr const char* kFunctionName = "dlopen";
  static void* func = dlsym(RTLD_NEXT, kFunctionName);

  USERVER_NAMESPACE::engine::impl::AssertDlFunctionFound(func, kFunctionName);
  USERVER_NAMESPACE::engine::impl::AssertDynamicLoadingEnabled(kFunctionName);

  return reinterpret_cast<DlOpenSignature>(func)(filename, flags);
}

#ifndef __clang__
[[gnu::visibility("default")]] [[gnu::externally_visible]]
#endif
// NOLINTNEXTLINE(readability-inconsistent-declaration-parameter-name)
void* dlmopen(Lmid_t lmid, const char *filename, int flags) {
  using DlMOpenSignature = void* (*)(Lmid_t, const char*, int);
  constexpr const char* kFunctionName = "dlmopen";
  static void* func = dlsym(RTLD_NEXT, "dlmopen");

  USERVER_NAMESPACE::engine::impl::AssertDlFunctionFound(func, kFunctionName);
  USERVER_NAMESPACE::engine::impl::AssertDynamicLoadingEnabled(kFunctionName);

  return reinterpret_cast<DlMOpenSignature>(func)(lmid, filename, flags);
}

#ifndef __clang__
[[gnu::visibility("default")]] [[gnu::externally_visible]]
#endif
// NOLINTNEXTLINE(readability-inconsistent-declaration-parameter-name)
int dlclose(void *handle) {
  using DlCloseSignature = int (*)(void*);
  constexpr const char* kFunctionName = "dlclose";
  static void* func = dlsym(RTLD_NEXT, "dlclose");

  USERVER_NAMESPACE::engine::impl::AssertDlFunctionFound(func, kFunctionName);
  USERVER_NAMESPACE::engine::impl::AssertDynamicLoadingEnabled(kFunctionName);

  return reinterpret_cast<DlCloseSignature>(func)(handle);
}
}

#else

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

void InitPhdrCacheAndDisableDynamicLoading() {}

void TeardownPhdrCacheAndEnableDynamicLoading() {}

}  // namespace engine::impl

USERVER_NAMESPACE_END

#endif
