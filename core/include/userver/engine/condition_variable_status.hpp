#pragma once

/// @file userver/engine/condition_variable_status.hpp
/// @brief @copybrief engine::CvStatus

USERVER_NAMESPACE_BEGIN

namespace engine {

/// std::condition_variable state extended with "cancelled" state
enum class CvStatus { kNoTimeout, kTimeout, kCancelled };

}  // namespace engine

USERVER_NAMESPACE_END
