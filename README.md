# μserver
Фреймворк для создания микросервисов.
Включает в себя:
  * Поддержку асинхронного кода на основе корутин
  * HTTP-сервер
  * Драйверы для СУБД: Redis, MongoDB

## Использование
**TBA**

## Разработка
Код должен соответствовать [Google C++ Style Guide](https://google.github.io/styleguide/cppguide) с [изменениями](https://wiki.yandex-team.ru/users/sermp/backend-cpp-codestyle/).
Перед коммитом необходимо прогнать clang-format.

### Сборка
Для сборки требуется компилятор с поддержкой C++14. В `Makefile` определены цели:
  * `all`/`build` -- выполняет сборку всего кода в директории
  * `clean` -- удаляет результаты сборки
  * `clang-format` -- выполняет запуск clang-format для всего кода
  * `smart-clang-format` -- clang-format только для измененных файлов
