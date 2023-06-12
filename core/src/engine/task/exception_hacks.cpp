#include <engine/task/exception_hacks.hpp>

#if defined(__has_feature)
#if __has_feature(thread_sanitizer)
#define HAS_TSAN 1
#endif
#if __has_feature(memory_sanitizer)
#include <sanitizer/msan_interface.h>
#define HAS_MSAN 1
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
#include <sys/mman.h>

#include <atomic>
#include <vector>

#include <fmt/format.h>

#include <userver/logging/log.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/impl/userver_experiments.hpp>
#include <utils/impl/assert_extra.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

namespace {

using DlIterateCb = int (*)(struct dl_phdr_info* info, size_t size, void* data);
using DlIterateFn = int (*)(DlIterateCb callback, void* data);

inline auto GetOriginalDlIteratePhdr() {
  static void* func = dlsym(RTLD_NEXT, "dl_iterate_phdr");
  if (!func) {
    utils::impl::AbortWithStacktrace(
        "Cannot find dl_iterate_phdr function with dlsym");
  }

  auto* original_func = reinterpret_cast<DlIterateFn>(func);

#ifdef HAS_MSAN
  // `dl_iterate_phdr` is not instrumented by MSAN. Returning our lambda
  // that unpoisons data before passing it to callback.
  return [original_func](DlIterateCb callback, void* data) {
    struct DataHolder {
      DlIterateCb callback;
      void* data;
    };

    DataHolder data_holder{callback, data};
    auto unpoisoning_callback = [](struct dl_phdr_info* info, size_t size,
                                   void* data) {
      __msan_unpoison(info, sizeof(*info));
      auto* data_holder = static_cast<DataHolder*>(data);
      UASSERT(data_holder);
      return data_holder->callback(info, size, data_holder->data);
    };

    return original_func(unpoisoning_callback, &data_holder);
  };
#else
  return original_func;
#endif
}

class MlockProcessDebugInfoScope final {
 public:
  void Initialize(const dl_phdr_info& self) {
    if (!Bootstrap(self)) {
      Teardown();
      LOG_WARNING() << "Failed to mlock(2) process debug info";
    } else {
      LOG_INFO() << "mlock(2)-ed approx " << mlocked_size_approx_
                 << " of process debug info";
    }
  }

  void Teardown() { MUnlockAndClear(); }

  ~MlockProcessDebugInfoScope() { Teardown(); }

 private:
  bool Bootstrap(const dl_phdr_info& self) {
    // we're fine with exceptions here and further down
    mlocked_sections_.reserve(self.dlpi_phnum);

    const auto image_base = self.dlpi_addr;
    for (decltype(self.dlpi_phnum) i = 0; i < self.dlpi_phnum; ++i) {
      const auto& phdr = self.dlpi_phdr[i];

      if (phdr.p_type == PT_LOAD || phdr.p_type == PT_GNU_EH_FRAME) {
        const void* start =
            reinterpret_cast<const void*>(image_base + phdr.p_vaddr);
        const auto len = phdr.p_memsz;

        if (!DoMlock(start, len)) {
          return false;
        } else {
          mlocked_sections_.push_back({start, len});
          mlocked_size_approx_ += len;
        }
      }
    }

    return true;
  }

  void MUnlockAndClear() {
    for (const auto& [start, len] : mlocked_sections_) {
      munlock(start, len);
    }
    mlocked_sections_.clear();
    mlocked_size_approx_ = 0;
  }

  static bool DoMlock(const void* start, std::size_t len) {
    if (mlock(start, len)) {
      const auto err = errno;

      LOG_WARNING() << "Failed to mlock(2) " << len << " bytes starting at "
                    << start << ", errno = " << err;
      return false;
    }

    return true;
  }

  struct Section final {
    const void* start;
    std::size_t len;
  };

  std::size_t mlocked_size_approx_{0};
  std::vector<Section> mlocked_sections_;
};

using PhdrCacheStorage = std::vector<dl_phdr_info>;
std::atomic<PhdrCacheStorage*> phdr_cache_ptr{nullptr};

class PhdrCache final {
 public:
  void Initialize() {
    if (!utils::impl::kPhdrCacheExperiment.IsEnabled()) {
      return;
    }
    LOG_INFO() << "Initializing phdr cache";

    GetOriginalDlIteratePhdr()(
        [](dl_phdr_info* info, size_t /* size */, void* data) {
          reinterpret_cast<PhdrCacheStorage*>(data)->push_back(*info);
          return 0;
        },
        &cache_);

    [[maybe_unused]] const auto* old_cache = phdr_cache_ptr.exchange(&cache_);
    UASSERT(!old_cache);

    LOG_INFO() << "Initialized phdr cache for " << cache_.size() << " objects";
  }

  static void Teardown() { phdr_cache_ptr.store(nullptr); }

  ~PhdrCache() { Teardown(); }

  static bool IsDynamicLoadingEnabled() {
    return phdr_cache_ptr.load() == nullptr;
  }

 private:
  PhdrCacheStorage cache_{};
};

MlockProcessDebugInfoScope& GetMlockProcessDebugInfoScope() noexcept {
  static MlockProcessDebugInfoScope mlock_debug_info_scope{};
  return mlock_debug_info_scope;
}

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

void InitPhdrCacheAndDisableDynamicLoading(DebugInfoAction debug_info_action) {
  static PhdrCache phdr_cache{};
  phdr_cache.Initialize();

  if (debug_info_action == DebugInfoAction::kLockInMemory) {
    DlIteratePhdr(
        [](struct dl_phdr_info* info, size_t, void*) {
          GetMlockProcessDebugInfoScope().Initialize(*info);
          // we are only interested in the first dl_phdr_info,
          // so stop iteration right away.
          return 1;
        },
        nullptr);
  }
}

void TeardownPhdrCacheAndEnableDynamicLoading() {
  PhdrCache::Teardown();
  GetMlockProcessDebugInfoScope().Teardown();
}

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

void InitPhdrCacheAndDisableDynamicLoading(DebugInfoAction) {}

void TeardownPhdrCacheAndEnableDynamicLoading() {}

}  // namespace engine::impl

USERVER_NAMESPACE_END

#endif
