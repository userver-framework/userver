#pragma once

#include <filesystem>
#include <type_traits>

#include <userver/utils/assert.hpp>
#include <userver/utils/underlying_value.hpp>

USERVER_NAMESPACE_BEGIN

namespace fs::blocking::impl {

template <typename Permissions>
concept LegacyPermissions = std::is_enum_v<Permissions> && requires {
    Permissions::perms_mask;
    Permissions::add_perms;
    Permissions::remove_perms;
    Permissions::symlink_perms;
};

template <LegacyPermissions Permissions>
constexpr std::filesystem::perms ToStdPermissions(Permissions perms) noexcept {
    return static_cast<
        std::filesystem::perms>(utils::UnderlyingValue(perms) & utils::UnderlyingValue(Permissions::perms_mask));
}

template <LegacyPermissions Permissions>
constexpr std::filesystem::perm_options ToStdPermOptions(Permissions perms) noexcept {
    const auto bits = utils::UnderlyingValue(perms);
    const bool add = (bits & utils::UnderlyingValue(Permissions::add_perms)) != 0;
    const bool remove = (bits & utils::UnderlyingValue(Permissions::remove_perms)) != 0;
    UASSERT_MSG(!(add && remove), "Permission flags for adding and removing permissions are mutually exclusive");

    auto options = std::filesystem::perm_options::replace;
    if (add) {
        options = std::filesystem::perm_options::add;
    } else if (remove) {
        options = std::filesystem::perm_options::remove;
    }
    if ((bits & utils::UnderlyingValue(Permissions::symlink_perms)) != 0) {
        options = options | std::filesystem::perm_options::nofollow;
    }
    return options;
}

}  // namespace fs::blocking::impl

USERVER_NAMESPACE_END
