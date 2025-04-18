#pragma once

/// @file userver/grpc-protovalidate/buf_validate.hpp
/// @brief Header which wraps @c buf/validate/validator.h to supress compiler
///        warnings from it.

#if defined(__GNUC__) && !defined(__clang__)
#pragma gcc system_header
#elif defined(__clang__)
#pragma clang system_header
#endif

#include <buf/validate/validator.h>
