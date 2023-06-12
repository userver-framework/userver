#include <userver/utils/impl/source_location.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::impl {

SourceLocation SourceLocation::Current(const char* filename,
                                       const char* function, size_t line) {
  return SourceLocation(filename, function, line);
}

SourceLocation::SourceLocation(const char* filename, const char* function,
                               size_t line)
    : file_name_(filename), function_name_(function), line_(line) {}

const char* SourceLocation::file_name() const { return file_name_; }

const char* SourceLocation::function_name() const { return function_name_; }

size_t SourceLocation::line() const { return line_; }

}  // namespace utils::impl

USERVER_NAMESPACE_END
