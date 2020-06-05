# μserver
Фреймворк для создания микросервисов.
Включает в себя:
  * Поддержку асинхронного кода на основе корутин
  * HTTP-сервер
  * Драйверы для СУБД: Redis, MongoDB, PostgreSQL

## Использование
[Документация API](https://github.yandex-team.ru/pages/taxi/userver/)

**TBA**

## Разработка
Код должен соответствовать [Google C++ Style Guide](https://h.yandex-team.ru/?https%3A//google.github.io/styleguide/cppguide) с [изменениями](https://wiki.yandex-team.ru/users/sermp/backend-cpp-codestyle/).
Код на Python должен соответствовать [Taxi Python Codestyle](https://wiki.yandex-team.ru/taxi/backend/codestyle/).
Перед коммитом необходимо прогнать clang-format.

Если изменения были в питоновских файлах, то желательно прогнать и `make check-pep8`.
Для проверки линтерами требуется поставить пакет `taxi-deps-py3-2` или 
воспользоваться `pip3 install -i https://pypi.yandex-team.ru/simple/ yandex-taxi-code-linters`
для установки в виртуальное окружение. Подробнее можно узнать [здесь](https://github.yandex-team.ru/taxi/code-linters/blob/master/README.md)

### Сборка

Для сборки требуются:
  * ОС:
    * Ubuntu Xenial c подключенным репозиторием xenial-updates/universe
    * Ubuntu Bionic c подключенным репозиторием bionic-updates/universe
    * MacOS 10.15 с установленными [Xcode](https://h.yandex-team.ru/?https%3A//apps.apple.com/us/app/xcode/id497799835) и [Homebrew](https://h.yandex-team.ru/?https%3A//brew.sh)
  * clang-7
  * clang-format-7

Минимальный набор зависимостей можно установить запуском скрипта `scripts/ubuntu-install-prerequisites.sh` для Ubuntu и `scripts/mac-os-install-prerequisites.sh` для MacOS.

В `Makefile` определены цели:
  * `all`/`build` -- выполняет сборку всего кода в директории
  * `build-release` -- выполняет сборку всего кода в релизе (полезно для запуска бенчмарков)
  * `clean` -- удаляет результаты сборки
  * `clang-format` -- выполняет запуск clang-format для всего кода
  * `smart-clang-format` -- clang-format только для измененных файлов
  * `check-pep8` -- выполняет проверку линтерами всех питоновских файлов
  * `smart-check-pep8` -- выполняет проверку линтерами всех измененных питоновских файлов
  * `teamcity-check-pep8` -- выполняет проверку линтерами всех питоновских файлов с выводом, адаптированным для teamcity
  * `taxi-yamlfmt` -- выполняет форматирование всех YAML файлов
  * `smart-taxi-yamlfmt` -- выполняет форматирование всех измененных YAML файлов
  * `taxi-jsonfmt` -- выполняет форматирование всех JSON файлов
  * `smart-taxi-jsonfmt` -- выполняет форматирование всех измененных JSON файлов
  * `taxi-eolfmt` --  проставляет отсутствующие символы окончания строки в конце файлов
  * `smart-taxi-eolfmt` --  проставляет отсутствующие символы окончания строки в конце измененных файлов
  * `format` -- выполняет форматирование всеми вышеперечисленными форматтерами
  * `smart-format` -- выполняет форматирование всех измененных файлов всеми вышеперечисленными форматтерами
  * `test` -- выполняет запуск тестов
  * `docs` -- выполняет сборку документации API
