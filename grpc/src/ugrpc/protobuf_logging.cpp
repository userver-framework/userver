#include <userver/ugrpc/protobuf_logging.hpp>

#include <google/protobuf/text_format.h>
#include <boost/container/small_vector.hpp>

#include <userver/utils/algo.hpp>
#include <userver/utils/meta_light.hpp>
#include <userver/utils/numeric_cast.hpp>

#include <userver/ugrpc/status_codes.hpp>
#include <userver/ugrpc/status_utils.hpp>

#include <ugrpc/impl/compat/protobuf_logging.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc {

namespace {

constexpr std::string_view kTruncateMarker = "...(truncated)";
constexpr std::string_view kEmptyMarker = "<EMPTY>";
constexpr std::string_view kNewLine = "\n";

class [[maybe_unused]] LimitingOutputStream final : public google::protobuf::io::ZeroCopyOutputStream {
public:
    class LimitReachedException final : public std::exception {};

    explicit LimitingOutputStream(google::protobuf::io::ArrayOutputStream& output_stream)
        : output_stream_{output_stream}
    {}

    /*
      Might throw `LimitReachedException` on limit reached
    */
    bool Next(void** data, int* size) override {
        if (!output_stream_.Next(data, size)) {
            limit_reached_ = true;
#if defined(ARCADIA_ROOT) || GOOGLE_PROTOBUF_VERSION >= 6031002
            // This requires TextFormat internals to be exception-safe, see
            // https://github.com/protocolbuffers/protobuf/commit/be875d0aaf37dbe6948717ea621278e75e89c9c7
            throw LimitReachedException{};
#endif
            return false;
        }
        return true;
    }

    void BackUp(int count) override {
        if (!limit_reached_) {
            output_stream_.BackUp(count);
        }
    }

    int64_t ByteCount() const override { return output_stream_.ByteCount(); }

    bool LimitReached() const { return limit_reached_; }

private:
    google::protobuf::io::ArrayOutputStream& output_stream_;
    bool limit_reached_{false};
};

template <typename T>
using SetRedactDebugStringConstraint = decltype(std::declval<T&>().SetRedactDebugString(true));

template <typename T>
constexpr bool kHasSetRedactDebugString = meta::IsDetected<SetRedactDebugStringConstraint, T>;

template <typename Printer = google::protobuf::TextFormat::Printer>
void Print(const google::protobuf::Message& message, google::protobuf::io::ZeroCopyOutputStream& output_stream) {
    if constexpr (kHasSetRedactDebugString<Printer>) {
        Printer printer;
        printer.SetUseUtf8StringEscaping(true);
        printer.SetExpandAny(true);
        printer.SetRedactDebugString(true);
        printer.Print(message, &output_stream);
    } else {
        impl::compat::Print(message, output_stream);
    }
}

}  // namespace

std::string ToLimitedDebugString(const google::protobuf::Message& message, std::size_t limit) {
    if (limit == 0) {
        return std::string{kTruncateMarker};
    }

    boost::container::small_vector<char, 1024> output_buffer{limit, boost::container::default_init};
    google::protobuf::io::ArrayOutputStream output_stream{output_buffer.data(), utils::numeric_cast<int>(limit)};

    LimitingOutputStream limiting_output_stream{output_stream};
    try {
        ugrpc::Print(message, limiting_output_stream);
    } catch (const LimitingOutputStream::LimitReachedException& /*ex*/) {
        // When using a protobuf version with exception-safe TextFormat, LimitingOutputStream throws
        // `LimitReachedException` if limit is reached to stop printing immediately, otherwise TextFormat will continue
        // to walk the whole message and apply noop printing.
    }

    std::string_view
        truncated_str = std::string_view{output_buffer.data(), static_cast<std::size_t>(output_stream.ByteCount())};
    UASSERT(truncated_str.size() <= limit);

    if (truncated_str.empty()) {
        return std::string{kEmptyMarker};
    }

    if (limiting_output_stream.LimitReached()) {
        if (truncated_str.size() <= kTruncateMarker.size() + kNewLine.size()) {
            return std::string{kTruncateMarker};
        }

        truncated_str.remove_suffix(kTruncateMarker.size() + kNewLine.size());
        return utils::StrCat(truncated_str, kNewLine, kTruncateMarker);
    }

    return std::string{truncated_str};
}

std::string ToUnlimitedDebugString(const google::protobuf::Message& message) {
    grpc::string result;
    google::protobuf::io::StringOutputStream output_stream(&result);
    ugrpc::Print(message, output_stream);
    std::string returned_str = std::string(result);
    if (returned_str.empty()) {
        return std::string{kEmptyMarker};
    }
    return returned_str;
}

std::string ToLimitedDebugString(const grpc::Status& status, std::size_t max_size) {
    if (status.ok()) {
        return "OK";
    }

    const auto gstatus = ugrpc::ToGoogleRpcStatus(status);
    if (gstatus.has_value()) {
        const std::string details_string = ugrpc::ToLimitedDebugString(*gstatus, max_size);
        return fmt::format(
            "code: {}, error message: {}\nerror details:\n{}",
            ugrpc::ToString(status.error_code()),
            status.error_message(),
            details_string
        );
    } else {
        return fmt::format("code: {}, error message: {}", ugrpc::ToString(status.error_code()), status.error_message());
    }
}

std::string ToUnlimitedDebugString(const grpc::Status& status) {
    if (status.ok()) {
        return "OK";
    }

    const auto gstatus = ugrpc::ToGoogleRpcStatus(status);
    if (gstatus.has_value()) {
        const std::string details_string = ugrpc::ToUnlimitedDebugString(*gstatus);
        return fmt::format(
            "code: {}, error message: {}\nerror details:\n{}",
            ugrpc::ToString(status.error_code()),
            status.error_message(),
            details_string
        );
    } else {
        return fmt::format("code: {}, error message: {}", ugrpc::ToString(status.error_code()), status.error_message());
    }
}

}  // namespace ugrpc

USERVER_NAMESPACE_END
