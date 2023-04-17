# userver: Third Party Libraries

This folder contains source codes from following open source projects:

* boost_stacktrace: https://github.com/boostorg/stacktrace/commit/7fedfa12654d18a9fa695de258763e93699c4636 (Boost 1.68.0)
* pfr: https://github.com/boostorg/pfr/releases/tag/2.0.3 with increased via `python misc/generate_cpp17.py 200 > include/boost/pfr/detail/core17_generated.hpp` fields count limit
* moodycamel: https://github.com/cameron314/concurrentqueue/releases/tag/v1.0.3
* rapidjson: https://github.com/Tencent/rapidjson/commit/083f359f5c36198accc2b9360ce1e32a333231d9 with RAPIDJSON_HAS_STDSTRING defined in rapidjson.h
* date: https://github.com/HowardHinnant/date/tree/v3.0.1
