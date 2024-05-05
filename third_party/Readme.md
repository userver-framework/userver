# userver: Third Party Libraries

This folder contains source codes from following open source projects:

* pfr: https://github.com/boostorg/pfr/releases/tag/2.2.0 with increased via `python misc/generate_cpp17.py 200 > include/boost/pfr/detail/core17_generated.hpp` fields count limit
* moodycamel: https://github.com/cameron314/concurrentqueue/releases/tag/v1.0.4
* rapidjson: https://github.com/Tencent/rapidjson/commit/083f359f5c36198accc2b9360ce1e32a333231d9 with RAPIDJSON_HAS_STDSTRING defined in rapidjson.h
* date: https://github.com/HowardHinnant/date/tree/v3.0.1
* uboost_coro: see uboost_coro/README.md
* llhttp: https://github.com/nodejs/llhttp/tree/release/v9.2.0
* http-parser: https://github.com/nodejs/http-parser/releases/tag/v2.9.4
