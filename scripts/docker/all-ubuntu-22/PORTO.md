==Как собрать docker и запустить проверки CI==

1. В директории лежит файл `./prepare_image.sh` - этот скриптик подготавливает 
директорию, для того, чтобы в ней собирать уже докер образ, а точнее, скачивает 
с гитхаба необходимые пакеты и устанавливает их версии

P.S.: Если нужно апнуть версию пакета в докере, достаточно изменить тег.

2. После успешной отработки скрипта, из п.1, нужно собрать докер образ командой:
```
docker build -t userver_docker_tag .
```
где `userver_docker_tag` нужно заменить на полный путь до пакета, 
если собираемся выкладывать докер образ во внешний гитхаб, например
`ghcr.io/userver-framework/ubuntu-userver-build-base:v1`.

3. Затем, необходимо запустить `docker run -d userver_docker_tag` для того, 
чтобы сформировался контейнер, который будем экспортировать в порто слой,
hash контейнера отобразиться в консоле

4. Экспортируем командой
```
docker export <hash_userver_docker_tag_container> -o ./layer.tar
```
сформируется слой, который уже можно выгрузить в sandbox

5. и выгружаем полученный слой командой
```
ya upload ./layer.tar -T PORTO_LAYER_STRM_TRNS \
    --owner taxi-common --ttl 90 \
    -d "taxi porto layer for compile userver with cmake" \
    --attr platform=linux_ubuntu_20.04_focal \
    --json-output
```
где `PORTO_LAYER_STRM_TRNS` - тег, обозначающий что мы выгружаем порто слой,
`owner` - владелец ресурса в sandbox, `ttl` - срок жизни ресурса в днях,
`d` - описание, `attr platform` - платформа, на какой будет запущен наш контейнер, 
`json-output` - вывод результата в `json` формате

Результатом будет примерно такое сообщение:
```
{
    "skynet_id": "rbtorrent:5e230328251ab8ef188c61266e502ee06942cc11",
    "task": {
        "url": "https://sandbox.yandex-team.ru/api/v1.0/task/1652353852",
        "id": 1652353852
    },
    "resource_id": 4172730048,
    "file_name": "layer.tar",
    "download_link": "https://proxy.sandbox.yandex-team.ru/4172730048",
    "resource_link": "https://sandbox.yandex-team.ru/resource/4172730048/view",
    "md5": "e850d1fa2ccb11c572154ef778967df8"
}
```
из которого нам нужен `resource_id`

6. Необходимо поменять ссылку на ресурс в [файле](https://a.yandex-team.ru/arcadia/taxi/uservices/userver/a.yaml),
в проверках CMake, нужно найти строчку `porto_layers` и указать там, полученный ранее `resource_id`, 
после чего проверки в CI (CMake) должны обновиться с изпользованием нового контейнера.

----
PS:
7. Чтобы обновить или создать новый докер образ на внешний [GitHub](https://github.com/userver-framework/userver), то необходимо
- иметь разрешение на запись [сюда](https://github.com/orgs/userver-framework/packages), (обратитесь к @antoshkka)
- необходимо получить токен для входа В интерфейсе GH, выберите Settings в профиле ->  Developer settings ->
Personal access tokens -> Generate new token (classic) и настройте токен, для деплоя и сохранения пакетов
!!! Сохраните значение токена

8. нужно залогиниться в ghcr.io
```
docker login -u <github_username> ghcr.io
```
в качестве пароля указать сохраненный выше токен

9. Отправить пакет на публикацию, например так:
```
docker push ghcr.io/userver-framework/ubuntu-userver-build-base:v1
```

10. После чего, необходимо проверить, что пакет виден для внешнего пользования:
выберите [тут](https://github.com/orgs/userver-framework/packages) загруженный пакет
выбрать Package settings и в Danger Zone выбрать Change package visibility - установить Public

11. Внести изменения в файл [docker-compose.yaml](https://a.yandex-team.ru/arcadia/taxi/uservices/userver/docker-compose.yml)
добавив или изменив контейнер для запуска докер образа для сборки userver
P.S.: стоит проверить, что github CI не сломался при внесении изменений, в случае необходимости, можно поправить и там.
