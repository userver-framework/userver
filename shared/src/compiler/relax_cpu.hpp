#pragma once

USERVER_NAMESPACE_BEGIN

namespace compiler {

class RelaxCpu final {
 public:
  // Should be called in spin-wait loops to reduce contention and allow other
  // threads to execute.
  void operator()() noexcept { __builtin_ia32_pause(); }
};

}  // namespace compiler

USERVER_NAMESPACE_END
