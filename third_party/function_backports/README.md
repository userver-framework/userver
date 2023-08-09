Imported from https://github.com/zhihaoy/nontype_functional/tree/p0792r13

The library was patched to support C++17 (C++20 was originally required).
Summary of the changes:

- used `namespace function_backports` instead of `namespace cpp23`
- usages of `requires` were replaced with `std::enable_if`
- `auto` function parameters were replaced with named template parameters
- poly-filled `std::type_identity`
- poly-filled `std::remove_cvref_t`
- added an extra `typename` where required by C++17
