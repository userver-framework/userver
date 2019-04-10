#include <engine/task/cancel.hpp>

#include <cctype>
#include <exception>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/core/demangle.hpp>
#include <boost/stacktrace.hpp>

#include <engine/sleep.hpp>
#include <logging/log.hpp>
#include <utils/assert.hpp>

#include <engine/task/coro_unwinder.hpp>
#include <engine/task/task_context.hpp>

namespace engine {
namespace {

// Heuristic to detect calls from destructors.
// It is not perfect as it is easily broken by destructor inlining, but it may
// help in some cases.
bool IsExecutingDestructor() {
  // https://itanium-cxx-abi.github.io/cxx-abi/abi.html#mangling-structure
  static const std::string kMangledNamePrefix = "_Z";

  for (const auto& frame : boost::stacktrace::stacktrace{}) {
    auto func_name = frame.name();

    // Remove compiler-specific suffixes
    // https://itanium-cxx-abi.github.io/cxx-abi/abi.html#mangling-general
    {
      auto pos = func_name.find('$');
      if (pos != std::string::npos) {
        func_name.resize(pos);
      }
    }

    if (boost::algorithm::starts_with(func_name, kMangledNamePrefix)) {
      // XXX: Demangler in xenial doesn't know about decltype(auto),
      // change it to auto. We may change some names, but they're not relevant.
      // https://itanium-cxx-abi.github.io/cxx-abi/abi.html#mangling-builtin
      boost::algorithm::replace_all(func_name, "Dc", "Da");
      func_name = boost::core::demangle(func_name.c_str());
    }

    if (!boost::algorithm::starts_with(func_name, kMangledNamePrefix)) {
      // Look for dtors (~Name)
      for (auto pos = func_name.find('~');
           pos != std::string::npos && pos + 1 < func_name.size();
           pos = func_name.find('~', pos + 1)) {
        // operator~()
        if (func_name[pos + 1] == '(') continue;
        // nested members (lambdas, classes etc.)
        if (func_name.find(':', pos + 1) != std::string::npos) continue;

        return true;
      }
    } else {
      // For some mysterious reason name wasn't demangled, using safe mode.
      // Look for "D*" pattern, where * is digit (actually 0, 1 or 2)
      // https://itanium-cxx-abi.github.io/cxx-abi/abi.html#mangling-special-ctor-dtor
      for (auto pos = func_name.find('D');
           pos != std::string::npos && pos + 1 < func_name.size();
           pos = func_name.find('D', pos + 1)) {
        // definitely not mangled dtor token
        if (!std::isdigit(func_name[pos + 1])) continue;

        return true;
      }
    }
  }
  return false;
}

void Unwind() {
  auto ctx = current_task::GetCurrentTaskContext();
  UASSERT(ctx->GetState() == Task::State::kRunning);

  if (std::uncaught_exception()) return;
  if (IsExecutingDestructor()) {
    LOG_WARNING() << "Attempt to cancel a task while executing destructor"
                  << logging::LogExtra::Stacktrace();
    return;
  }
  if (ctx->SetCancellable(false)) {
    LOG_TRACE() << "Cancelling current task" << logging::LogExtra::Stacktrace();
    // NOLINTNEXTLINE(hicpp-exception-baseclass)
    throw impl::CoroUnwinder{};
  }
}

}  // namespace

TaskCancellationBlocker::TaskCancellationBlocker()
    : context_(current_task::GetCurrentTaskContext()),
      was_allowed_(context_->SetCancellable(false)) {}

TaskCancellationBlocker::~TaskCancellationBlocker() {
  UASSERT(context_ == current_task::GetCurrentTaskContext());
  context_->SetCancellable(was_allowed_);
}

namespace current_task {

bool IsCancelRequested() {
  return GetCurrentTaskContext()->IsCancelRequested();
}

bool ShouldCancel() {
  const auto ctx = GetCurrentTaskContext();
  return ctx->IsCancelRequested() && ctx->IsCancellable();
}

void CancellationPoint() {
  if (current_task::ShouldCancel()) Unwind();
}

}  // namespace current_task
}  // namespace engine
