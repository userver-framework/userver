#include <benchmark/benchmark.h>

#include <userver/fs/blocking/c_file.hpp>
#include <userver/fs/blocking/temp_directory.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

template <class T>
void WriteTrivial(fs::blocking::CFile& writer, T value) {
    static_assert(std::is_trivially_copyable_v<T>);
    writer.Write(std::string_view{reinterpret_cast<const char*>(&value), sizeof(value)});
}

// Code close to WriteInteger from userver/core/src/dump/common.cpp
void WriteInteger(fs::blocking::CFile& writer, std::uint64_t value) {
    if (value < 0x80) {
        WriteTrivial(writer, static_cast<std::uint8_t>(value));
    } else if (value < 0x4000) {
        WriteTrivial(writer, static_cast<std::uint16_t>(value | 0x8000));
    } else if (value < 0x3f00'0000) {
        WriteTrivial(writer, static_cast<std::uint32_t>(value | 0xc000'0000));
    } else {
        WriteTrivial(writer, std::uint8_t{0xff});
        WriteTrivial(writer, value);
    }
}

void CFileIntegersWrite(benchmark::State& state) {
    const auto dir = fs::blocking::TempDirectory::Create();
    const std::string path = dir.GetPath() + "/foo";

    fs::blocking::CFile file(path, {fs::blocking::OpenFlag::kWrite, fs::blocking::OpenFlag::kCreateIfNotExists});
    for ([[maybe_unused]] auto _ : state) {
        for (std::uint64_t value = 1; value < 0xf000'0000; value *= 10) {
            WriteInteger(file, value);
        }
    }
}

}  // namespace

BENCHMARK(CFileIntegersWrite);

USERVER_NAMESPACE_END
