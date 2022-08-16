# uboost_coro
Подмножество [Boost](https://www.boost.org) версии 1.80.0, включающее библиотеки Coroutine2 и Context.
В качестве основы для `CMakeLists.txt` использован [boost-cmake](https://github.com/Orphis/boost-cmake).

Поддерживает сборку с `USERVER_SANITIZE` (в частности, AddressSanitizer и
ThreadSanitizer).

## Изменения по сравнению с upstream
  * Директория `boost` переименована в `uboost_coro`, и перемещена в `include`.
  Инклуды из `context` и `coroutine2` замкнуты на новые пути.
  * Исходники перемещены из `libs/<libname>/src` в `src/<libname>`.
  * Убраны файлы, относящиеся к некоторым неподдерживающимся архитектурам.
