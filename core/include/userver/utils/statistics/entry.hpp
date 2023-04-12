#pragma once

/// @file userver/utils/statistics/entry.hpp
/// @brief Header with all the types required in component header to use
/// statistics (includes utils::statistics::Entry and forward declarations).

#include <userver/utils/fast_pimpl.hpp>
#include <userver/utils/statistics/fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {

namespace impl {
enum class UnregisteringKind { kManual, kAutomatic };
}

/// @brief Statistics registration holder, used to unregister a metric source
/// before it is destroyed.
///
/// See utils::statistics::Storage for info on registrations
class [[nodiscard]] Entry final {
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
