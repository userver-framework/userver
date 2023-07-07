#pragma once

#include <memory>

USERVER_NAMESPACE_BEGIN

namespace utils::impl {

class DisableCoreDumps final {
 public:
  DisableCoreDumps();

  DisableCoreDumps(DisableCoreDumps&&) = delete;
  DisableCoreDumps& operator=(DisableCoreDumps&&) = delete;
  ~DisableCoreDumps();

  bool IsValid() const noexcept;
  void Invalidate() noexcept;

 private:
  class Impl;

  std::unique_ptr<Impl> impl_;
};

}  // namespace utils::impl

USERVER_NAMESPACE_END
