#include <logging/impl/formatters/struct.hpp>

#include <chrono>

#include <fmt/compile.h>
#include <fmt/format.h>

#include <logging/timestamp.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging::impl::formatters {

Struct::Struct(Level level, LogClass log_class, const utils::impl::SourceLocation& location) {
    item_.level = level;
    item_.log_class = log_class;
    item_.location = location;
}

void Struct::AddTag(std::string_view key, const LogExtra::Value& value) { item_.tags.emplace_back(key, value); }

void Struct::AddTag(std::string_view key, std::string_view value) { item_.tags.emplace_back(key, std::string{value}); }

void Struct::SetText(std::string_view text) { item_.text = text; }

LoggerItemRef Struct::ExtractLoggerItem() { return item_; }

}  // namespace logging::impl::formatters

USERVER_NAMESPACE_END
