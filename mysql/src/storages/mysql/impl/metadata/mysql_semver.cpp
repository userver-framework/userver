#include <storages/mysql/impl/metadata/mysql_semver.hpp>

#include <tuple>

#include <fmt/format.h>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::impl::metadata {

namespace {

std::uint16_t NextPart(std::uint64_t& version) {
  const std::uint16_t part{static_cast<std::uint16_t>(version % 100)};
  version /= 100;
  return part;
}

}  // namespace

MySQLSemVer::MySQLSemVer(std::uint16_t major, std::uint16_t minor,
                         std::uint16_t patch) noexcept
    : patch_{patch}, minor_{minor}, major_{major} {}

MySQLSemVer::MySQLSemVer(std::uint64_t version) noexcept
    : patch_{NextPart(version)},
      minor_{NextPart(version)},
      major_{NextPart(version)} {}

std::uint16_t MySQLSemVer::Major() const noexcept { return major_; }
std::uint16_t MySQLSemVer::Minor() const noexcept { return minor_; }
std::uint16_t MySQLSemVer::Patch() const noexcept { return patch_; }

std::string MySQLSemVer::ToString() const {
  return fmt::format("{}.{}.{}", Major(), Minor(), Patch());
}

bool MySQLSemVer::operator==(const MySQLSemVer& other) const noexcept {
  return std::tie(major_, minor_, patch_) ==
         std::tie(other.major_, other.minor_, other.patch_);
}

bool MySQLSemVer::operator<(const MySQLSemVer& other) const noexcept {
  return std::tie(major_, minor_, patch_) <
         std::tie(other.major_, other.minor_, other.patch_);
}

bool MySQLSemVer::operator>(const MySQLSemVer& other) const noexcept {
  return other < *this;
}

bool MySQLSemVer::operator<=(const MySQLSemVer& other) const noexcept {
  return !(other < *this);
}

bool MySQLSemVer::operator>=(const MySQLSemVer& other) const noexcept {
  return !(*this < other);
}

}  // namespace storages::mysql::impl::metadata

USERVER_NAMESPACE_END
