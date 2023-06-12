#pragma once

#include <cstdint>
#include <string>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::impl::metadata {

class SemVer final {
 public:
  SemVer(std::uint16_t major, std::uint16_t minor,
         std::uint16_t patch) noexcept;
  SemVer(std::uint64_t version) noexcept;

  std::uint16_t Major() const noexcept;
  std::uint16_t Minor() const noexcept;
  std::uint16_t Patch() const noexcept;

  std::string ToString() const;

  bool operator==(const SemVer& other) const noexcept;
  bool operator<(const SemVer& other) const noexcept;
  bool operator>(const SemVer& other) const noexcept;
  bool operator<=(const SemVer& other) const noexcept;
  bool operator>=(const SemVer& other) const noexcept;

 private:
  std::uint16_t major_;
  std::uint16_t minor_;
  std::uint16_t patch_;
};

}  // namespace storages::mysql::impl::metadata

USERVER_NAMESPACE_END
