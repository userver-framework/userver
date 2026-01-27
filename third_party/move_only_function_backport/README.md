Imported from libc++ PR ["Implement std::move_only_function"](https://github.com/llvm/llvm-project/pull/94670).
Specifically, from the [commit](https://github.com/llvm/llvm-project/pull/94670/commits/7a203002b19f5a2827607e73a998dcd1ace9d135) on June 6, 2024.

The library was patched to support C++17 (C++20 was originally required).

Summary of the changes:

- used `namespace function_backports`
- removed `_LIBCPP_STD_VER` guards
- used `#pragma once`
- replaced `_LIBCPP_` macro prefix with `USERVER_`
- removed `_LIBCPP_HIDE_FROM_ABI`
- replaced internal libc++ assert macros with `assert()`
- replaced usages of `requires` with `std::enable_if`
- added `std::` where required
- replaced internal libc++ includes with standard includes
- removed a usage of sized `delete[]`
- replaced `auto` function parameters with named template parameters
- inlined `std::decay_t<_VT>` in `__is_callable_from` to work around a compiler bug
- added an extra `typename` where required by C++17
