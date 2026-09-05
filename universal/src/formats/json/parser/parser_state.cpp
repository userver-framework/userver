#include <userver/formats/json/parser/parser_state.hpp>

#include <boost/container/small_vector.hpp>

#include <fmt/format.h>
#include <rapidjson/error/en.h>
#include <rapidjson/reader.h>

#include <userver/formats/common/path.hpp>
#include <userver/formats/json/parser/base_parser.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/algo.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/fast_scope_guard.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::json::parser {

namespace {

constexpr std::size_t kMaxSourceFragmentLength = 128;

std::string MakeDepthLimitErrorMessage() {
    return fmt::format("Exceeded maximum allowed JSON depth of: {}", kDepthParseLimit);
}

rapidjson::ParseErrorCode GetLegacyCompatibleParseErrorCode(
    rapidjson::ParseErrorCode error_code,
    std::size_t error_offset,
    std::string_view json_input
) {
    constexpr std::string_view kJsonWhitespace = " \n\r\t";
    if (error_code != rapidjson::kParseErrorValueInvalid) {
        return error_code;
    }

    const auto first_token_offset = json_input.find_first_not_of(kJsonWhitespace);
    if (first_token_offset == std::string_view::npos || error_offset != first_token_offset) {
        return error_code;
    }

    switch (json_input[error_offset]) {
        case ']':
        case '}':
        case ',':
        case ':':
            return rapidjson::kParseErrorDocumentEmpty;
        default:
            return error_code;
    }
}

}  // namespace

struct ParserState::Impl {
    struct StackItem final {
        BaseParser* parser;
    };

    class ParserStackSaxHandler;

    // JSON in handlers is often 2-5 items in depth
    boost::container::small_vector<StackItem, 16> stack;
    bool is_processing_input{false};

    void PushParser(BaseParser& parser, ParserState& parser_state);

    [[nodiscard]] std::string GetPath() const;
};

class ParserState::Impl::ParserStackSaxHandler final {
public:
    ParserStackSaxHandler(Impl& parser_state_impl, rapidjson::MemoryStream& input_stream, std::string_view json_input)
        : parser_state_impl_(parser_state_impl),
          input_stream_(input_stream),
          json_input_(json_input)
    {}

    bool Null() {
        return DispatchEvent<SaxEventKind::kValue>([](BaseParser& current_parser) { current_parser.Null(); });
    }

    bool Bool(bool value) {
        return DispatchEvent<SaxEventKind::kValue>([value](BaseParser& current_parser) { current_parser.Bool(value); });
    }

    bool Int(int64_t value) { return Int64(value); }
    bool Uint(uint64_t value) { return Uint64(value); }

    bool Int64(int64_t value) {
        return DispatchEvent<SaxEventKind::kValue>([value](BaseParser& current_parser) { current_parser.Int64(value); }
        );
    }

    bool Uint64(uint64_t value) {
        return DispatchEvent<SaxEventKind::kValue>([value](BaseParser& current_parser) { current_parser.Uint64(value); }
        );
    }

    bool Double(double value) {
        return DispatchEvent<SaxEventKind::kValue>([value](BaseParser& current_parser) { current_parser.Double(value); }
        );
    }

    bool StartObject() {
        return DispatchEvent<SaxEventKind::kContainerStart>([](BaseParser& current_parser) {
            current_parser.StartObject();
        });
    }

    bool EndObject(std::size_t member_count) {
        return DispatchEvent<SaxEventKind::kContainerEnd>([member_count](BaseParser& current_parser) {
            current_parser.EndObject(member_count);
        });
    }

    bool StartArray() {
        return DispatchEvent<SaxEventKind::kContainerStart>([](BaseParser& current_parser) {
            current_parser.StartArray();
        });
    }

    bool EndArray(std::size_t element_count) {
        return DispatchEvent<SaxEventKind::kContainerEnd>([element_count](BaseParser& current_parser) {
            current_parser.EndArray(element_count);
        });
    }

    bool Key(const char* key_data, std::size_t key_size, bool) {
        return DispatchEvent<SaxEventKind::kKey>([key_data, key_size](BaseParser& current_parser) {
            current_parser.Key(std::string_view{key_data, key_size});
        });
    }

    bool String(const char* value_data, std::size_t value_size, bool) {
        return DispatchEvent<SaxEventKind::kValue>([value_data, value_size](BaseParser& current_parser) {
            current_parser.String(std::string_view{value_data, value_size});
        });
    }

    bool RawNumber(const char*, std::size_t, bool) noexcept { return false; }

    [[nodiscard]] std::size_t GetErrorOffset() const {
        return callback_error_offset_ != std::string_view::npos ? callback_error_offset_ : input_stream_.Tell();
    }

    [[nodiscard]] std::string BuildSourceFragmentSuffix(std::size_t fragment_end_offset) const {
        constexpr std::string_view kLatestTokenPrefix = ", the latest token was ";
        constexpr std::string_view kTruncatedSuffix = "... (truncated)";

        UASSERT(previous_event_end_offset_ <= fragment_end_offset);
        UASSERT(fragment_end_offset <= json_input_.size());

        if (fragment_end_offset == previous_event_end_offset_) {
            return {};
        }

        const auto source_fragment =
            json_input_.substr(previous_event_end_offset_, fragment_end_offset - previous_event_end_offset_);
        if (source_fragment.size() > kMaxSourceFragmentLength) {
            return utils::StrCat(
                kLatestTokenPrefix,
                source_fragment.substr(0, kMaxSourceFragmentLength),
                kTruncatedSuffix
            );
        }

        return utils::StrCat(kLatestTokenPrefix, source_fragment);
    }

private:
    enum class SaxEventKind {
        kValue,
        kKey,
        kContainerStart,
        kContainerEnd,
    };

    template <SaxEventKind EventKind, typename Callback>
    bool DispatchEvent(Callback dispatch_to_parser) {
        const auto event_end_offset = input_stream_.Tell();
        constexpr bool is_structural_event =
            EventKind == SaxEventKind::kContainerStart || EventKind == SaxEventKind::kContainerEnd;
        UASSERT(!is_structural_event || event_end_offset > 0);

        const auto callback_error_offset = event_end_offset - static_cast<std::size_t>(is_structural_event);
        utils::FastScopeGuard set_callback_error_offset_on_failure{[this, callback_error_offset]() noexcept {
            callback_error_offset_ = callback_error_offset;
        }};
        dispatch_to_parser(GetCurrentParser());
        set_callback_error_offset_on_failure.Release();

        UpdateStateAfterEvent<EventKind>(event_end_offset);
        previous_event_end_offset_ = event_end_offset;
        return true;
    }

    BaseParser& GetCurrentParser() const {
        UASSERT(!parser_state_impl_.stack.empty());
        UASSERT(parser_state_impl_.stack.back().parser);
        return *parser_state_impl_.stack.back().parser;
    }

    template <SaxEventKind EventKind>
    void UpdateStateAfterEvent(std::size_t event_end_offset) {
        bool is_document_complete = false;
        if constexpr (EventKind == SaxEventKind::kValue) {
            is_document_complete = open_container_count_ == 0;
        } else if constexpr (EventKind == SaxEventKind::kContainerStart) {
            ++open_container_count_;
        } else if constexpr (EventKind == SaxEventKind::kContainerEnd) {
            UASSERT(open_container_count_ > 0);
            --open_container_count_;
            is_document_complete = open_container_count_ == 0;
        }

        if (is_document_complete) {
            return;
        }

        if (parser_state_impl_.stack.empty()) [[unlikely]] {
            throw ParseError(
                event_end_offset,
                parser_state_impl_.GetPath(),
                "Symbols after end of document" + BuildSourceFragmentSuffix(event_end_offset)
            );
        }

        if (open_container_count_ > kDepthParseLimit || parser_state_impl_.stack.size() > kDepthParseLimit) [[unlikely]]
        {
            throw ParseError(
                event_end_offset,
                parser_state_impl_.GetPath(),
                MakeDepthLimitErrorMessage() + BuildSourceFragmentSuffix(event_end_offset)
            );
        }
    }

    Impl& parser_state_impl_;
    rapidjson::MemoryStream& input_stream_;
    std::string_view json_input_;
    std::size_t previous_event_end_offset_{0};
    std::size_t callback_error_offset_{std::string_view::npos};
    std::size_t open_container_count_{0};
};

void ParserState::Impl::PushParser(BaseParser& parser, ParserState& parser_state) {
    parser.SetState(parser_state);
    stack.push_back({&parser});
}

std::string ParserState::Impl::GetPath() const {
    std::string result;

    for (const auto& item : stack) {
        const auto str = item.parser->GetPathItem();
        if (str.empty()) {
            continue;
        }

        if (!result.empty()) {
            result += '.';
        }
        result += str;
    }

    return result;
}

ParserState::ParserState() = default;

ParserState::~ParserState() = default;

void ParserState::PushParser(BaseParser& parser) { impl_->PushParser(parser, *this); }

void ParserState::ProcessInput(std::string_view input) {
    rapidjson::Reader reader;
    rapidjson::MemoryStream input_stream(input.data(), input.size());
    auto& stack = impl_->stack;

    UASSERT_MSG(!impl_->is_processing_input, "ParserState::ProcessInput must not be called recursively");
    impl_->is_processing_input = true;
    const utils::FastScopeGuard reset_processing_flag{[this]() noexcept { impl_->is_processing_input = false; }};

    Impl::ParserStackSaxHandler handler(*impl_, input_stream, input);

    if (stack.empty()) {
        throw ParseError(input_stream.Tell(), impl_->GetPath(), "Symbols after end of document");
    }

    if (stack.size() > kDepthParseLimit) {
        throw ParseError(input_stream.Tell(), impl_->GetPath(), MakeDepthLimitErrorMessage());
    }

    try {
        static constexpr auto kParseFlags = static_cast<
            rapidjson::ParseFlag>(rapidjson::kParseDefaultFlags | rapidjson::kParseFullPrecisionFlag);
        reader.Parse<kParseFlags>(input_stream, handler);
        if (reader.HasParseError()) {
            const auto error_offset = reader.GetErrorOffset();
            const auto error_code = GetLegacyCompatibleParseErrorCode(reader.GetParseErrorCode(), error_offset, input);
            throw ParseError{
                error_offset,
                impl_->GetPath(),
                rapidjson::GetParseError_En(error_code),
            };
        }
    } catch (const ParseError&) {
        throw;
    } catch (const std::exception& e) {
        const auto cur_pos = handler.GetErrorOffset();
        throw ParseError{
            cur_pos,
            impl_->GetPath(),
            e.what() + handler.BuildSourceFragmentSuffix(cur_pos),
        };
    }

    if (input_stream.Tell() != input.size()) {
        throw ParseError(
            input_stream.Tell(),
            "",
            rapidjson::GetParseError_En(rapidjson::ParseErrorCode::kParseErrorDocumentRootNotSingular)
        );
    }

    if (!stack.empty()) {
        throw ParseError(input_stream.Tell(), "", "data is expected after the end of file");
    }
}

std::string ParserState::GetCurrentPath() const { return impl_->GetPath(); }

BaseParser& ParserState::GetTopParser() const {
    UASSERT(!impl_->stack.empty());
    UASSERT(impl_->stack.back().parser);
    return *impl_->stack.back().parser;
}

void ParserState::PopMe([[maybe_unused]] BaseParser& parser) {
    UASSERT(!impl_->stack.empty());
    UASSERT(&parser == impl_->stack.back().parser);

    impl_->stack.pop_back();
}

}  // namespace formats::json::parser

USERVER_NAMESPACE_END
