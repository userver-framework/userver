#pragma once

#include <userver/utils/flags.hpp>

/// @file userver/fs/settings_read_file.hpp
/// @brief settings flags for directory recursive read operations

USERVER_NAMESPACE_BEGIN

namespace fs {

/// @brief filesystem support
enum class SettingsReadFile {
    kNone = 0,
    /// Skip hidden files,
    kSkipHidden = 1 << 0,
};

using SettingReadFileFlags = utils::Flags<SettingsReadFile>;

}  // namespace fs

USERVER_NAMESPACE_END
