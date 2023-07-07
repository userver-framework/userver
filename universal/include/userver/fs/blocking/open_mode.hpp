#pragma once

/// @file userver/fs/blocking/open_mode.hpp
/// @brief @copybrief fs::blocking::OpenMode

#include <userver/utils/flags.hpp>

USERVER_NAMESPACE_BEGIN

namespace fs::blocking {

enum class OpenFlag {
  kNone = 0,

  /// Open the file for reading. Reading starts at the beginning of the file.
  /// At least one of `kRead`, `kWrite` must be set.
  kRead = 1 << 0,

  /// Open the file for writing. Writing starts at the beginning of the file.
  /// By default, the file won't be created if it doesn't already exist.
  /// At least one of `kRead`, `kWrite` must be set.
  kWrite = 1 << 1,

  /// Used together with `kWrite` to create an empty file and open it
  /// for writing if it doesn't already exist.
  kCreateIfNotExists = 1 << 2,

  /// Differs from `kCreateIfNotExists` in that it ensures that the 'open'
  /// operation creates the file.
  kExclusiveCreate = 1 << 3,

  /// Used together with `kWrite` to clear the contents of the file in case it
  /// already exists.
  kTruncate = 1 << 4,

  /// Used together with `kWrite` to open file for writing to the end of the
  /// file.
  kAppend = 1 << 5,
};

/// A set of OpenFlags
using OpenMode = utils::Flags<OpenFlag>;

}  // namespace fs::blocking

USERVER_NAMESPACE_END
