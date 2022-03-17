#pragma once

#include <userver/utils/clang_format_workarounds.hpp>
#include <userver/utils/fast_pimpl.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {

class Storage;

class USERVER_NODISCARD Entry final {
 public:
  Entry();

  Entry(const Entry& other) = delete;
  Entry& operator=(const Entry& other) = delete;
  Entry(Entry&& other) noexcept;
  Entry& operator=(Entry&& other) noexcept;
  ~Entry();

  void Unregister() noexcept;

 private:
  struct Impl;

  friend class Storage;  // in RegisterExtender()

  explicit Entry(const Impl& impl) noexcept;

  utils::FastPimpl<Impl, 16, 8> impl_;
};

}  // namespace utils::statistics

USERVER_NAMESPACE_END
