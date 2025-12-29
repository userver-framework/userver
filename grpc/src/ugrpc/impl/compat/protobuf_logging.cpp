#include <ugrpc/impl/compat/protobuf_logging.hpp>

#include <fmt/format.h>

#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/text_format.h>
#include <grpcpp/support/config.h>

#include <userver/compiler/thread_local.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl::compat {

namespace {

[[maybe_unused]] bool HasDebugRedactOption([[maybe_unused]] const google::protobuf::FieldDescriptor& field) {
#if defined(ARCADIA_ROOT) || GOOGLE_PROTOBUF_VERSION >= 4022000
    return field.options().debug_redact();
#else
    return false;
#endif
}

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
        bool single_line_mode,
        BaseTextGenerator* generator
    ) const override {
        PrintRedacted(generator);
        if (single_line_mode) {
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

    void Print(const google::protobuf::Message& message, google::protobuf::io::ZeroCopyOutputStream& output_stream)
        const {
        printer_.Print(message, &output_stream);
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
                fmt::format("Failed to register field value printer for field: '{}'", field.full_name())
            };
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

}  // namespace ugrpc::impl::compat

USERVER_NAMESPACE_END
