#include <userver/utest/log_capture_fixture.hpp>

#include <algorithm>
#include <mutex>

#include <fmt/format.h>
#include <boost/algorithm/cxx11/all_of.hpp>

#include <userver/utils/algo.hpp>
#include <userver/utils/encoding/tskv_parser.hpp>
#include <userver/utils/encoding/tskv_parser_read.hpp>
#include <userver/utils/text_light.hpp>

USERVER_NAMESPACE_BEGIN

namespace utest {

namespace {}  // namespace

namespace impl {

class ToStringLogger : public logging::impl::LoggerBase {
public:
    ToStringLogger(logging::Format format) : logging::impl::LoggerBase(format) {
        UINVARIANT(
            format == logging::Format::kTskv || format == logging::Format::kRaw,
            "Parsing for this logging::Format is not supported"
        );
        logging::impl::LoggerBase::SetLevel(logging::Level::kInfo);
    }

    void Log(logging::Level level, std::string_view str) override {
        const std::lock_guard lock{mutex_};
        records_.push_back(LogRecord{utils::impl::InternalTag{}, level, std::string{str}});
    }

    std::vector<LogRecord> GetAll() const {
        const std::lock_guard lock{mutex_};
        return records_;
    }

    std::vector<LogRecord> Filter(utils::function_ref<bool(const LogRecord&)> predicate) const {
        const std::lock_guard lock{mutex_};
        std::vector<LogRecord> result;
        for (const auto& record : records_) {
            if (predicate(record)) {
                result.push_back(record);
            }
        }
        return result;
    }

    void Clear() noexcept {
        const std::lock_guard lock{mutex_};
        records_.clear();
    }

private:
    std::vector<LogRecord> records_;
    mutable std::mutex mutex_;
};

}  // namespace impl

const std::string& LogRecord::GetText() const { return GetTag("text"); }

const std::string& LogRecord::GetTag(std::string_view key) const {
    auto tag_value = GetTagOrNullptr(key);
    if (!tag_value) {
        throw std::runtime_error(fmt::format("No '{}' tag in log record:\n{}", key, log_raw_));
    }
    return *std::move(tag_value);
}

std::optional<std::string> LogRecord::GetTagOptional(std::string_view key) const {
    if (const auto* const value = GetTagOrNullptr(key)) {
        return *value;
    }
    return std::nullopt;
}

const std::string* LogRecord::GetTagOrNullptr(std::string_view key) const {
    const auto iter = std::find_if(tags_.begin(), tags_.end(), [&](const std::pair<std::string, std::string>& tag) {
        return tag.first == key;
    });

    if (iter != tags_.end()) {
        return &iter->second;
    }

    return nullptr;
}

const std::string& LogRecord::GetLogRaw() const { return log_raw_; }

logging::Level LogRecord::GetLevel() const { return level_; }

LogRecord::LogRecord(utils::impl::InternalTag, logging::Level level, std::string&& log_raw)
    : level_(level), log_raw_(std::move(log_raw)) {
    utils::encoding::TskvParser parser{log_raw_};

    const auto on_invalid_record = [&] {
        throw std::runtime_error(fmt::format("Invalid log record: \"{}\"", log_raw_));
    };

    const char* const record_begin = parser.SkipToRecordBegin();
    if (!record_begin || record_begin != log_raw_.data()) {
        on_invalid_record();
    }

    const auto status = utils::encoding::TskvReadRecord(parser, [&](const std::string& key, const std::string& value) {
        tags_.emplace_back(key, value);
        return true;
    });

    if (status == utils::encoding::TskvParser::RecordStatus::kIncomplete) {
        on_invalid_record();
    }
    if (parser.GetStreamPosition() != log_raw_.data() + log_raw_.size()) {
        on_invalid_record();
    }
}

std::ostream& operator<<(std::ostream& os, const LogRecord& data) {
    os << data.GetLogRaw();
    return os;
}

std::ostream& operator<<(std::ostream& os, const std::vector<LogRecord>& data) {
    for (const auto& record : data) {
        os << record;
    }
    return os;
}

LogRecord GetSingleLog(utils::span<const LogRecord> log, const utils::impl::SourceLocation& source_location) {
    if (log.size() != 1) {
        std::string msg =
            fmt::format("There are {} log records instead of 1 at {}:\n", log.size(), ToString(source_location));
        for (const auto& record : log) {
            msg += record.GetLogRaw();
        }
        throw NotSingleLogError(msg);
    }
    auto single_record = std::move(log[0]);
    return single_record;
}

LogCaptureLogger::LogCaptureLogger(logging::Format format)
    : logger_(utils::MakeSharedRef<impl::ToStringLogger>(format)) {}

logging::LoggerPtr LogCaptureLogger::GetLogger() const { return logger_.GetBase(); }

std::vector<LogRecord> LogCaptureLogger::GetAll() const { return logger_->GetAll(); }

std::vector<LogRecord> LogCaptureLogger::Filter(
    std::string_view text_substring,
    utils::span<const std::pair<std::string_view, std::string_view>> tag_substrings
) const {
    return Filter([&](const LogRecord& record) {
        return record.GetText().find(text_substring) != std::string_view::npos &&
               boost::algorithm::all_of(tag_substrings, [&](const auto& kv) {
                   const auto* tag_value = record.GetTagOrNullptr(kv.first);
                   return tag_value && tag_value->find(kv.second) != std::string_view::npos;
               });
    });
}

std::vector<LogRecord> LogCaptureLogger::Filter(utils::function_ref<bool(const LogRecord&)> predicate) const {
    return logger_->Filter(predicate);
}

void LogCaptureLogger::Clear() noexcept { logger_->Clear(); }

}  // namespace utest

USERVER_NAMESPACE_END
