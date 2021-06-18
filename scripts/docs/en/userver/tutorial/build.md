## Build and setup


## Step by step guide

@todo Fill me

### Download

### Build

@todo remove the yandex-taxi- prefix

```
bash
mkdir build_release
cd build_release
cmake -DCMAKE_BUILD_TYPE=Release ..
make yandex-taxi-userver-core_unittest
```

### Run tests
```
bash
./userver/core/yandex-taxi-userver-core_unittest
```
