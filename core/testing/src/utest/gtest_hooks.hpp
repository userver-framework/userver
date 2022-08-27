#pragma once

#include <userver/logging/level.hpp>

USERVER_NAMESPACE_BEGIN

namespace utest::impl {

/// These functions should be called ASAP (before test framework initialization)
///@{
void FinishStaticInit();
///@}

/// These functions should be called before RUN_ALL_TESTS.
///@{
void InitMockNow();

void SetLogLevel(logging::Level);
///@}

}  // namespace utest::impl

USERVER_NAMESPACE_END
