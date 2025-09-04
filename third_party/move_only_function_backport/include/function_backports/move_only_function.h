//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#pragma once

#pragma GCC system_header

// move_only_function design:
//
// move_only_function has a small buffer with a size of `3 * sizeof(void*)` bytes. This buffer can only be used when the
// object that should be stored is trivially relocatable (currently only when it is trivially move constructible and
// trivially destructible). There is also a bool in the lower bits of the vptr stored which is set when the contained
// object is not trivially destructible.
//
// trivially relocatable: It would also be possible to store nothrow_move_constructible types, but that would mean
// that move_only_function itself would not be trivially relocatable anymore. The decision to keep move_only_function
// trivially relocatable was made because we expect move_only_function to be mostly used to store a functor. To only
// forward functors there is std::function_ref (not voted in yet, expected in C++26).
//
// buffer size: We did a survey of six implementations from various vendors. Three of them had a buffer size of 24 bytes
// on 64 bit systems. This also allows storing a std::string or std::vector inside the small buffer (once the compiler
// has full support of trivially_relocatable annotations).
//
// trivially-destructible bit: This allows us to keep the overall binary size smaller because we don't have to store
// a pointer to a noop function inside the vtable. It also avoids loading the vtable during destruction, potentially
// resulting in fewer cache misses. The downside is that calling the function now also requires setting the lower bits
// of the pointer to zero, but this is a very fast operation on modern CPUs.

// NOLINTBEGIN(readability-duplicate-include)
#  define USERVER_IN_MOVE_ONLY_FUNCTION_H

#  include "move_only_function_impl.h"

#  define USERVER_MOVE_ONLY_FUNCTION_REF &
#  include "move_only_function_impl.h"

#  define USERVER_MOVE_ONLY_FUNCTION_REF &&
#  include "move_only_function_impl.h"

#  define USERVER_MOVE_ONLY_FUNCTION_CV const
#  include "move_only_function_impl.h"

#  define USERVER_MOVE_ONLY_FUNCTION_CV const
#  define USERVER_MOVE_ONLY_FUNCTION_REF &
#  include "move_only_function_impl.h"

#  define USERVER_MOVE_ONLY_FUNCTION_CV const
#  define USERVER_MOVE_ONLY_FUNCTION_REF &&
#  include "move_only_function_impl.h"

#  define USERVER_MOVE_ONLY_FUNCTION_NOEXCEPT true
#  include "move_only_function_impl.h"

#  define USERVER_MOVE_ONLY_FUNCTION_NOEXCEPT true
#  define USERVER_MOVE_ONLY_FUNCTION_REF &
#  include "move_only_function_impl.h"

#  define USERVER_MOVE_ONLY_FUNCTION_NOEXCEPT true
#  define USERVER_MOVE_ONLY_FUNCTION_REF &&
#  include "move_only_function_impl.h"

#  define USERVER_MOVE_ONLY_FUNCTION_NOEXCEPT true
#  define USERVER_MOVE_ONLY_FUNCTION_CV const
#  include "move_only_function_impl.h"

#  define USERVER_MOVE_ONLY_FUNCTION_NOEXCEPT true
#  define USERVER_MOVE_ONLY_FUNCTION_CV const
#  define USERVER_MOVE_ONLY_FUNCTION_REF &
#  include "move_only_function_impl.h"

#  define USERVER_MOVE_ONLY_FUNCTION_NOEXCEPT true
#  define USERVER_MOVE_ONLY_FUNCTION_CV const
#  define USERVER_MOVE_ONLY_FUNCTION_REF &&
#  include "move_only_function_impl.h"

#  undef USERVER_IN_MOVE_ONLY_FUNCTION_H
// NOLINTEND(readability-duplicate-include)
