#pragma once

#include <cstdint>
#include <string>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::impl::metadata {

class MySQLSemVer final {
 public:
  MySQLSemVer(std::uint16_t major, std::uint16_t minor,
              std::uint16_t patch) noexcept;
  MySQLSemVer(std::uint64_t version) noexcept;

  std::uint16_t Major() const noexcept;
  std::uint16_t Minor() const noexcept;
  std::uint16_t Patch() const noexcept;

  std::string ToString() const;

  bool operator==(const MySQLSemVer& other) const noexcept;
  bool operator<(const MySQLSemVer& other) const noexcept;
  bool operator>(const MySQLSemVer& other) const noexcept;
  bool operator<=(const MySQLSemVer& other) const noexcept;
  bool operator>=(const MySQLSemVer& other) const noexcept;

 private:
  std::uint16_t patch_;
  std::uint16_t minor_;
  std::uint16_t major_;
};

}  // namespace storages::mysql::impl::metadata

USERVER_NAMESPACE_END