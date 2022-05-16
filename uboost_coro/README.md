# μboost_coro
Подмножество [Boost](https://www.boost.org) версии 1.70.0, включающее библиотеки Coroutine2 и Context.
В качестве основы для `CMakeLists.txt` использован [boost-cmake](https://github.com/Orphis/boost-cmake).

Поддерживает сборку с `SANITIZE` (в частности, AddressSanitizer).

## Изменения по сравнению с upstream
  * Директория `boost` переименована в `uboost_coro`, и перемещена в `include`.
  Инклуды из `context` и `coroutine2` замкнуты на новые пути.
  * Исходники перемещены из `libs/<libname>/src` в `src/<libname>`.
  * Убраны файлы, относящиеся к не-`x86` архитектурам и Windows.
  * В [stack_traits.cpp](src/context/posix/stack_traits.cpp) вставлен костыль,
  исправляющий сборку `std::call_once` из libstdc++ 5.4 в clang.
