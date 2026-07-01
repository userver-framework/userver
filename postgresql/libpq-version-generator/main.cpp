#include <fstream>

#include <libpq-fe.h>

int main() {
    const int version = PQlibVersion();
    std::ofstream out("libpq_version/userver_libpq_version.hpp", std::ios::out | std::ios::trunc);
    out << "#pragma once\n"
        << "\n"
        << "#define USERVER_LIBPQ_VERSION " << version << "\n"
        << R"~~(
#if USERVER_LIBPQ_VERSION < 140000
#error libpq must be at least version 14.0 if building with CMake option -DUSERVER_FEATURE_PATCH_LIBPQ=ON
#endif
)~~";

    return 0;
}
