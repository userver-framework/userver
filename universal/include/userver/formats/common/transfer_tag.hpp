#pragma once

/// @file userver/formats/common/transfer_tag.hpp
/// @brief @copybrief formats::common::TransferTag

USERVER_NAMESPACE_BEGIN

namespace formats::common {

/// @brief This tag class is used to move a `ValueBuilder` object without
/// copying
///
/// Unlike the move-constructor, if the object points to a subtree, only the
/// pointer is copied. Note that storing the resulting `ValueBuilder` in a
/// container for a long period of time will cause a "memory leak": the child
/// `ValueBuilder` will hold the entire original tree, starting from the root.
class TransferTag final {};

}  // namespace formats::common

USERVER_NAMESPACE_END
