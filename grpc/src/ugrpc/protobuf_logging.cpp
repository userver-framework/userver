#include <userver/ugrpc/protobuf_logging.hpp>

#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/text_format.h>
#include <boost/container/small_vector.hpp>

#include <userver/compiler/thread_local.hpp>
#include <userver/ugrpc/status_codes.hpp>
#include <userver/ugrpc/status_utils.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/numeric_cast.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl {

namespace {

[[maybe_unused]] bool HasDebugRedactOption([[maybe_unused]] const google::protobuf::FieldDescriptor& field) {
#if defined(ARCADIA_ROOT) || GOOGLE_PROTOBUF_VERSION >= 4022000
    return field.options().debug_redact();
#else
    return false;
#endif
}

class [[maybe_unused]] LimitingOutputStream final : public google::protobuf::io::ZeroCopyOutputStream {
public:
    class LimitReachedException final : public std::exception {};

    explicit LimitingOutputStream(google::protobuf::io::ArrayOutputStream& output_stream)
        : output_stream_{output_stream} {}

    /*
      Throw `LimitReachedException` on limit reached
    */
    bool Next(void** data, int* size) override {
        if (!output_stream_.Next(data, size)) {
            limit_reached_ = true;
            // This requires TextFormat internals to be exception-safe, see
            // https://github.com/protocolbuffers/protobuf/commit/be875d0aaf37dbe6948717ea621278e75e89c9c7
            throw LimitReachedException{};
        }
        return true;
    }

    void BackUp(int count) override {
        if (!limit_reached_) {
            output_stream_.BackUp(count);
        }
    }

    int64_t ByteCount() const override { return output_stream_.ByteCount(); }

private:
    google::protobuf::io::ArrayOutputStream& output_stream_;
    bool limit_reached_{false};
};

class DebugRedactFieldValuePrinter final : public google::protobuf::TextFormat::FastFieldValuePrinter {
public:
    using BaseTextGenerator = google::protobuf::TextFormat::BaseTextGenerator;

    void PrintBool(bool, BaseTextGenerator* generator) const override { PrintRedacted(generator); }

    void PrintInt32(std::int32_t, BaseTextGenerator* generator) const override { PrintRedacted(generator); }

    void PrintUInt32(std::uint32_t, BaseTextGenerator* generator) const override { PrintRedacted(generator); }

#if defined(ARCADIA_ROOT)
    void PrintInt64(long, BaseTextGenerator* generator) const override { PrintRedacted(generator); }

    void PrintUInt64(unsigned long, BaseTextGenerator* generator) const override { PrintRedacted(generator); }
#else
    void PrintInt64(std::int64_t, BaseTextGenerator* generator) const override { PrintRedacted(generator); }

    void PrintUInt64(std::uint64_t, BaseTextGenerator* generator) const override { PrintRedacted(generator); }
#endif

    void PrintFloat(float, BaseTextGenerator* generator) const override { PrintRedacted(generator); }

    void PrintDouble(double, BaseTextGenerator* generator) const override { PrintRedacted(generator); }

    void PrintString(const grpc::string&, BaseTextGenerator* generator) const override { PrintRedacted(generator); }

    void PrintBytes(const grpc::string&, BaseTextGenerator* generator) const override { PrintRedacted(generator); }

    void PrintEnum(std::int32_t, const grpc::string&, BaseTextGenerator* generator) const override {
        PrintRedacted(generator);
    }

    bool PrintMessageContent(
        const google::protobuf::Message& /*message*/,
        int /*fieldIndex*/,
        int /*fieldCount*/,
        bool singleLineMode,
        BaseTextGenerator* generator
    ) const override {
        PrintRedacted(generator);
        if (singleLineMode) {
            generator->PrintLiteral(" ");
        } else {
            generator->PrintLiteral("\n");
        }
        // don't use default printing logic
        return true;
    }

    void PrintMessageEnd(
        const google::protobuf::Message& /*message*/,
        int /*field_index*/,
        int /*field_count*/,
        bool /*single_line_mode*/,
        BaseTextGenerator* /*generator*/
    ) const override {
        // noop
    }

    void PrintMessageStart(
        const google::protobuf::Message& /*message*/,
        int /*field_index*/,
        int /*field_count*/,
        bool /*single_line_mode*/,
        BaseTextGenerator* generator
    ) const override {
        generator->PrintLiteral(": ");
    }

private:
    void PrintRedacted(BaseTextGenerator* generator) const { generator->PrintLiteral("[REDACTED]"); }
};

class DebugStringPrinter {
public:
    DebugStringPrinter() {
        printer_.SetUseUtf8StringEscaping(true);
        printer_.SetExpandAny(true);
    }

    void Print(const google::protobuf::Message& message, google::protobuf::io::ZeroCopyOutputStream& stream) const {
        printer_.Print(message, &stream);
    }

    void RegisterDebugRedactPrinters(const google::protobuf::Descriptor& desc) { VisitMessageRecursive(desc); }

private:
    void VisitMessageRecursive(const google::protobuf::Descriptor& desc) {
        const auto [_, inserted] = registered_messages_.insert(&desc);
        if (inserted) {
            for (int i = 0; i < desc.field_count(); ++i) {
                const google::protobuf::FieldDescriptor* field = desc.field(i);
                UINVARIANT(field, "field is nullptr");
                VisitField(*field);
            }
        }
    }

    void VisitField(const google::protobuf::FieldDescriptor& field) {
        if (HasDebugRedactOption(field)) {
            RegisterDebugRedactFieldValuePrinter(field);
        } else {
            const google::protobuf::Descriptor* msg = field.message_type();
            if (msg) {
                VisitMessageRecursive(*msg);
            }
        }
    }

    void RegisterDebugRedactFieldValuePrinter(const google::protobuf::FieldDescriptor& field) {
        auto field_value_printer = std::make_unique<DebugRedactFieldValuePrinter>();
        if (printer_.RegisterFieldValuePrinter(&field, field_value_printer.get())) {
            // RegisterFieldValuePrinter takes ownership of the printer on successful registration
            [[maybe_unused]] const auto p = field_value_printer.release();
        } else {
            throw std::runtime_error{
                fmt::format("Failed to register field value printer for field: '{}'", field.full_name())};
        }
    }

    google::protobuf::TextFormat::Printer printer_;
    std::unordered_set<const google::protobuf::Descriptor*> registered_messages_;
};

compiler::ThreadLocal kDebugStringPrinter = [] { return DebugStringPrinter{}; };

}  // namespace

void Print(const google::protobuf::Message& message, google::protobuf::io::ZeroCopyOutputStream& output_stream) {
    auto printer = kDebugStringPrinter.Use();
    printer->RegisterDebugRedactPrinters(*message.GetDescriptor());
    printer->Print(message, output_stream);
}

}  // namespace ugrpc::impl

namespace ugrpc {

std::string ToLimitedDebugString(const google::protobuf::Message& message, std::size_t limit) {
    boost::container::small_vector<char, 1024> output_buffer{limit, boost::container::default_init};
    google::protobuf::io::ArrayOutputStream output_stream{output_buffer.data(), utils::numeric_cast<int>(limit)};

    auto printer = impl::kDebugStringPrinter.Use();
    printer->RegisterDebugRedactPrinters(*message.GetDescriptor());

#if defined(ARCADIA_ROOT) || GOOGLE_PROTOBUF_VERSION >= 6031002
    // Throw `LimitReachedException` on limit reached to stop printing immediately, otherwise TextFormat will continue
    // to walk the whole message and apply noop printing.
    impl::LimitingOutputStream limiting_output_stream{output_stream};
    try {
        impl::Print(message, limiting_output_stream);
    } catch (const impl::LimitingOutputStream::LimitReachedException& /*ex*/) {
        // Buffer limit has been reached.
    }
#else
    // For old protobuf, we cannot apply hard limits when printing messages, because its TextFormat is not
    // exception-safe. https://github.com/protocolbuffers/protobuf/commit/be875d0aaf37dbe6948717ea621278e75e89c9c7
    impl::Print(message, output_stream);
#endif
    std::string returned_str = std::string{output_buffer.data(), static_cast<std::size_t>(output_stream.ByteCount())};
    if (returned_str.empty() && limit >= 7) return "<EMPTY>";
    return returned_str;
}

std::string ToUnlimitedDebugString(const google::protobuf::Message& message) {
    grpc::string result;
    google::protobuf::io::StringOutputStream output_stream(&result);
    impl::Print(message, output_stream);
    std::string returned_str = std::string(result);
    if (returned_str.empty()) return "<EMPTY>";
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
