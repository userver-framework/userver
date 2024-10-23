#include <fstream>

#include <libpq-fe.h>

int main() {
    const int version = PQlibVersion();
    std::ofstream out("libpq_version/userver_libpq_version.hpp", std::ios::out | std::ios::trunc);
    out << "#pragma once\n"
        << "\n"
        << "#define USERVER_LIBPQ_VERSION " << version << "\n";
    return 0;
}
