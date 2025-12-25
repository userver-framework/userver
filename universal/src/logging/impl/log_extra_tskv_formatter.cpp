#include <userver/logging/impl/log_extra_tskv_formatter.hpp>

#include <stdexcept>

#include <boost/container/small_vector.hpp>
#include <boost/exception/diagnostic_information.hpp>

#include <userver/utils/assert.hpp>

#include <logging/impl/formatters/tskv.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging::impl {

namespace {

TskvPtr CreateFormatter(Format format) {
    switch (format) {
        case Format::kRaw:
        case Format::kLtsv:
        case Format::kTskv:
            return utils::Box<impl::formatters::Tskv>(Level::kDebug, format, utils::impl::SourceLocation::Current());

        case Format::kJson:
        case Format::kJsonYaDeploy:
            throw std::runtime_error("Invalid LogExtraTskvFormatter format: kJson and kJsonYaDeploy are not supported");
    }

    throw std::runtime_error("Invalid LogExtraTskvFormatter format");
}

}  // namespace

LogExtraTskvFormatter::LogExtraTskvFormatter(logging::Format format)
    : formatter_(CreateFormatter(format))
{}

LogExtraTskvFormatter::LogExtraTskvFormatter(LogExtraTskvFormatter&&) noexcept = default;

LogExtraTskvFormatter& LogExtraTskvFormatter::operator=(LogExtraTskvFormatter&&) noexcept = default;

LogExtraTskvFormatter::~LogExtraTskvFormatter() = default;

TextLogItem& LogExtraTskvFormatter::GetTextLogItem() {
    auto& item = formatter_->GetLoggerItem();

    UASSERT(dynamic_cast<logging::impl::TextLogItem*>(&item));

    // Safe: Tskv formatter always creates TextLogItem, verified by dynamic_cast above.
    // The approach is taken from lsp-logger.cpp.
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
    return static_cast<logging::impl::TextLogItem&>(item);
}

TextLogItem& LogExtraTskvFormatter::ExtractTextLogItem() {
    auto& item = formatter_->ExtractLoggerItem();

    UASSERT(dynamic_cast<logging::impl::TextLogItem*>(&item));

    // Safe: Tskv formatter always creates TextLogItem, verified by dynamic_cast above.
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
    return static_cast<logging::impl::TextLogItem&>(item);
}

void LogExtraTskvFormatter::Append(const LogExtra& log_extra) {
    if (log_extra.extra_->empty()) {
        return;
    }

    for (const auto& [key, value] : *log_extra.extra_) {
        formatter_->AddTag(key, value.GetValue());
    }
}

}  // namespace logging::impl

USERVER_NAMESPACE_END
