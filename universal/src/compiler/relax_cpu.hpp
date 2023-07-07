#pragma once

USERVER_NAMESPACE_BEGIN

namespace compiler {

class RelaxCpu final {
 public:
  // Should be called in spin-wait loops to reduce contention and allow other
  // threads to execute.
  void operator()() noexcept {
#if defined(__x86_64__) || defined(__amd64__) || defined(__i386__)
    __builtin_ia32_pause();
#elif defined(__aarch64__)
    __asm__ __volatile__("yield");
#else
#error "Unsupported CPU architecture"
#endif
  }
};

}  // namespace compiler

USERVER_NAMESPACE_END
