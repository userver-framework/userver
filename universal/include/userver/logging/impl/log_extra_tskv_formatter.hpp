#pragma once

#include <string_view>

#include <userver/logging/format.hpp>
#include <userver/logging/impl/formatters/base.hpp>
#include <userver/logging/impl/logger_base.hpp>
#include <userver/logging/log_extra.hpp>
#include <userver/utils/box.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging::impl {

namespace formatters {
class Tskv;
}  // namespace formatters

using TskvPtr = utils::Box<formatters::Tskv>;

class LogExtraTskvFormatter final {
public:
    explicit LogExtraTskvFormatter(logging::Format format);

    LogExtraTskvFormatter(LogExtraTskvFormatter&&) noexcept;

    LogExtraTskvFormatter& operator=(LogExtraTskvFormatter&&) noexcept;

    ~LogExtraTskvFormatter();

    void Append(const LogExtra& log_extra);

    // Returns log line
    TextLogItem& GetTextLogItem();

    // Finishes log line and returns it
    TextLogItem& ExtractTextLogItem();

private:
    TskvPtr formatter_;
};

}  // namespace logging::impl

USERVER_NAMESPACE_END
