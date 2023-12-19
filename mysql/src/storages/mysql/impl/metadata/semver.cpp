#include <storages/mysql/impl/metadata/semver.hpp>

#include <tuple>

#include <fmt/format.h>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::impl::metadata {

namespace {

std::uint16_t GetMajor(std::uint64_t version) { return version / 100 / 100; }

std::uint16_t GetMinor(std::uint64_t version) { return (version / 100) % 100; }

std::uint16_t GetPatch(std::uint64_t version) { return version % 100; }

}  // namespace

SemVer::SemVer(std::uint16_t major, std::uint16_t minor,
               std::uint16_t patch) noexcept
    : major_{major}, minor_{minor}, patch_{patch} {}

SemVer::SemVer(std::uint64_t version) noexcept
    : SemVer{GetMajor(version), GetMinor(version), GetPatch(version)} {}

std::uint16_t SemVer::Major() const noexcept { return major_; }
std::uint16_t SemVer::Minor() const noexcept { return minor_; }
std::uint16_t SemVer::Patch() const noexcept { return patch_; }

std::string SemVer::ToString() const {
  return fmt::format("{}.{}.{}", Major(), Minor(), Patch());
}

bool SemVer::operator==(const SemVer& other) const noexcept {
  return std::tie(major_, minor_, patch_) ==
         std::tie(other.major_, other.minor_, other.patch_);
}

bool SemVer::operator<(const SemVer& other) const noexcept {
  return std::tie(major_, minor_, patch_) <
         std::tie(other.major_, other.minor_, other.patch_);
}

bool SemVer::operator>(const SemVer& other) const noexcept {
  return other < *this;
}

bool SemVer::operator<=(const SemVer& other) const noexcept {
  return !(other < *this);
}

bool SemVer::operator>=(const SemVer& other) const noexcept {
  return !(*this < other);
}

}  // namespace storages::mysql::impl::metadata

USERVER_NAMESPACE_END
