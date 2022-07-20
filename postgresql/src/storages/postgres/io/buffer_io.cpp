#include <userver/storages/postgres/io/buffer_io.hpp>

#include <atomic>
#include <iostream>
#include <typeindex>
#include <unordered_map>

#include <fmt/format.h>

#include <userver/compiler/demangle.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres::io::detail {

#ifndef NDEBUG

namespace {

struct TypeAndInstantiationFiles {
  std::type_index type;
  std::vector<const char*> files{};
};

using TypeToIO =
    std::unordered_map<std::type_index, std::vector<TypeAndInstantiationFiles>>;

TypeToIO& GetReaders() {
  static TypeToIO map;
  return map;
}

TypeToIO& GetWriters() {
  static TypeToIO map;
  return map;
}

void InsertNew(TypeToIO& map, std::type_index type, std::type_index io_type,
               const char* base_file) {
  base_file = base_file + logging::impl::PathBaseSize(base_file);

  auto it = map.find(type);
  if (it != map.end()) {
    for (auto& known_io : it->second) {
      if (known_io.type == io_type) {
        known_io.files.push_back(base_file);
        return;
      }
    }
  }

  map[type].push_back({io_type, {base_file}});
}

std::string FormProblems(const TypeToIO& map, const char* check_info) {
  std::string result;

  for (const auto& [type, ios] : map) {
    for (std::size_t i = 1; i < ios.size(); ++i) {
      result += fmt::format(
          "Type '{}' has conflicting instantiation of {}: "
          "'{}' vs '{}' in base files [{}] vs [{}]\n",
          compiler::GetTypeName(type), check_info,
          compiler::GetTypeName(ios[0].type),
          compiler::GetTypeName(ios[i].type), fmt::join(ios[0].files, ", "),
          fmt::join(ios[i].files, ", "));
    }
  }

  return result;
}

void ReportOdrProblems() {
  static std::atomic<bool> was_checked{false};
  if (was_checked) return;
  if (was_checked.exchange(true)) return;

  auto odr_parser_problems = FormProblems(GetReaders(), "parsers");
  auto odr_formatter_problems = FormProblems(GetWriters(), "formatters");
  if (!odr_parser_problems.empty() || !odr_formatter_problems.empty()) {
    auto msg = fmt::format(
        "{}{}\nProbably you forgot to include parser/serializer before the "
        "point of PostgrSQL usage.",
        odr_parser_problems, odr_formatter_problems);

    LOG_ERROR() << msg;
    logging::LogFlush();
    std::cerr << msg;
    std::abort();
  }
}

}  // namespace

ReadersRegistrator::ReadersRegistrator(std::type_index type,
                                       std::type_index parser_type,
                                       const char* base_file) {
  InsertNew(GetReaders(), type, parser_type, base_file);
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
void ReadersRegistrator::RequireInstance() const { ReportOdrProblems(); }

WritersRegistrator::WritersRegistrator(std::type_index type,
                                       std::type_index formatter_type,
                                       const char* base_file) {
  InsertNew(GetWriters(), type, formatter_type, base_file);
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
void WritersRegistrator::RequireInstance() const { ReportOdrProblems(); }

#endif

}  // namespace storages::postgres::io::detail

USERVER_NAMESPACE_END
