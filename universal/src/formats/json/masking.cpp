#include <userver/formats/json/masking.hpp>

#include <algorithm>
#include <optional>
#include <utility>

#include <fmt/format.h>
#include <rapidjson/error/en.h>
#include <rapidjson/memorystream.h>
#include <rapidjson/reader.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <boost/container/small_vector.hpp>

#include <userver/formats/json/exception.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/impl/transparent_hash.hpp>
#include <userver/utils/text_light.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::json {
namespace {

struct Node final {
    utils::impl::TransparentMap<std::string, std::size_t> children;
    std::optional<std::size_t> wildcard;
    bool terminal{false};
};

enum class Action { kCopy, kTraverse, kMask };

enum class Container { kObject, kArray };

struct State final {
    const Node* node;
    Action action;
};

class MaskingHandler final : public rapidjson::BaseReaderHandler<rapidjson::UTF8<>, MaskingHandler> {
public:
    MaskingHandler(
        const std::vector<Node>& nodes,
        MaskingListMode mode,
        rapidjson::StringBuffer& output,
        std::size_t limit
    )
        : nodes_(nodes),
          mode_(mode),
          output_(output),
          limit_(limit),
          writer_(output),
          pending_(Select(&nodes.front()))
    {}

    bool Null() {
        return Scalar([this] { return writer_.Null(); });
    }
    bool Bool(bool value) {
        return Scalar([this, value] { return writer_.Bool(value); });
    }
    bool String(const char* value, rapidjson::SizeType length, bool) {
        return Scalar([this, value, length] { return writer_.String(value, GetStringLengthForBudget(value, length)); });
    }
    bool RawNumber(const char* value, rapidjson::SizeType length, bool) {
        return Scalar([this, value, length] {
            return writer_.RawValue(
                value,
                static_cast<rapidjson::SizeType>(std::min<std::size_t>(length, RemainingBytes())),
                rapidjson::kNumberType
            );
        });
    }

    bool Key(const char* value, rapidjson::SizeType length, bool) {
        const auto state = stack_.back().state;
        if (state.action == Action::kMask) {
            return true;
        }
        if (!writer_.Key(value, GetStringLengthForBudget(value, length)) || !CheckLimit()) {
            return false;
        }
        if (state.action == Action::kCopy) {
            pending_ = state;
        } else {
            const auto found = state.node->children.find(std::string_view{value, length});
            const auto
                child = found != state.node->children.end() ? std::optional{found->second} : state.node->wildcard;
            pending_ = Select(child ? &nodes_[*child] : nullptr);
        }
        return true;
    }

    bool StartObject() { return StartContainer(Container::kObject); }
    bool StartArray() { return StartContainer(Container::kArray); }
    bool EndObject(rapidjson::SizeType) { return EndContainer(Container::kObject); }
    bool EndArray(rapidjson::SizeType) { return EndContainer(Container::kArray); }

    bool IsLimitReached() const { return limit_reached_; }

private:
    struct Frame final {
        State state;
        Container kind;
    };

    std::size_t RemainingBytes() const { return limit_ - std::min(limit_, output_.GetSize()); }

    rapidjson::SizeType GetStringLengthForBudget(const char* value, rapidjson::SizeType length) const {
        const auto remaining = RemainingBytes();
        if (length <= remaining) {
            return length;
        }
        auto prefix = std::string_view{value, remaining};
        utils::text::utf8::TrimViewTruncatedEnding(prefix);
        const auto prefix_length =
            prefix.size() == remaining
                ? remaining
                : prefix.size() + utils::text::utf8::CodePointLengthByFirstByte(value[prefix.size()]);
        return static_cast<rapidjson::SizeType>(prefix_length);
    }

    bool CheckLimit() {
        limit_reached_ = output_.GetSize() >= limit_;
        return !limit_reached_;
    }

    State Select(const Node* node) const {
        if (node && !node->terminal && node->children.empty() && !node->wildcard) {
            node = nullptr;
        }
        if (node && !node->terminal) {
            return {node, Action::kTraverse};
        }
        const bool selected = node && node->terminal;
        const bool mask = mode_ == MaskingListMode::kBlacklist ? selected : !selected;
        return {node, mask ? Action::kMask : Action::kCopy};
    }

    State Current() const {
        if (!stack_.empty() && (stack_.back().kind == Container::kArray || stack_.back().state.action == Action::kMask))
        {
            return stack_.back().state;
        }
        return pending_;
    }

    template <typename Write>
    bool Scalar(Write write) {
        if (!stack_.empty() && stack_.back().state.action == Action::kMask) {
            return true;
        }
        const auto state = Current();
        if (state.action == Action::kMask ||
            (state.action == Action::kTraverse && mode_ == MaskingListMode::kWhitelist))
        {
            return writer_.String("...") && CheckLimit();
        }
        return write() && CheckLimit();
    }

    bool StartContainer(Container kind) {
        const bool is_array = kind == Container::kArray;
        if (stack_.size() >= kDepthParseLimit) {
            throw ParseException("JSON masking depth limit exceeded");
        }
        const bool suppressed = !stack_.empty() && stack_.back().state.action == Action::kMask;
        const auto state = Current();
        if (!suppressed) {
            bool written = false;
            if (state.action == Action::kMask) {
                written = writer_.String(is_array ? "[...]" : "{...}");
            } else if (is_array) {
                written = writer_.StartArray();
            } else {
                written = writer_.StartObject();
            }
            if (!written || !CheckLimit()) {
                return false;
            }
        }
        stack_.push_back({state, kind});
        return true;
    }

    bool EndContainer(Container kind) {
        const bool is_array = kind == Container::kArray;
        UASSERT(!stack_.empty());
        const auto state = stack_.back().state;
        stack_.pop_back();
        if (state.action != Action::kMask) {
            return (is_array ? writer_.EndArray() : writer_.EndObject()) && CheckLimit();
        }
        return true;
    }

    const std::vector<Node>& nodes_;
    const MaskingListMode mode_;
    rapidjson::StringBuffer& output_;
    const std::size_t limit_;
    rapidjson::Writer<rapidjson::StringBuffer> writer_;
    State pending_;
    boost::container::small_vector<Frame, 16> stack_;
    bool limit_reached_{false};
};

}  // namespace

struct MaskingRules::Impl final {
    std::vector<Node> nodes{1};
    MaskingListMode mode;

    Impl(const std::vector<MaskingPath>& paths, MaskingListMode list_mode)
        : mode(list_mode)
    {
        for (const auto& path : paths) {
            std::size_t index = 0;
            for (const auto& segment : path) {
                if (nodes[index].terminal) {
                    break;
                }
                std::optional<std::size_t> child;
                if (segment == "*") {
                    child = nodes[index].wildcard;
                    if (!child) {
                        nodes[index].wildcard = child = nodes.size();
                    }
                } else {
                    child = nodes[index].children.try_emplace(segment, nodes.size()).first->second;
                }
                if (*child == nodes.size()) {
                    nodes.emplace_back();
                }
                index = *child;
            }
            nodes[index].terminal = true;
        }
    }
};

MaskingRules::MaskingRules(const std::vector<MaskingPath>& paths, MaskingListMode mode)
    : impl_(std::make_shared<const Impl>(paths, mode))
{}

MaskedJson MaskJson(std::string_view input, const MaskingRules& rules, std::size_t log_limit) {
    if (log_limit == 0) {
        return {{}, true};
    }
    rapidjson::StringBuffer output;
    MaskingHandler handler(rules.impl_->nodes, rules.impl_->mode, output, log_limit);
    rapidjson::MemoryStream stream(input.data(), input.size());
    rapidjson::Reader reader;
    constexpr auto kFlags =
        rapidjson::kParseIterativeFlag | rapidjson::kParseValidateEncodingFlag | rapidjson::kParseNumbersAsStringsFlag;
    if (!reader.Parse<kFlags>(stream, handler)) {
        if (!handler.IsLimitReached() || reader.GetParseErrorCode() != rapidjson::kParseErrorTermination) {
            throw ParseException(fmt::format(
                "JSON masking parse error at byte {}: {}",
                reader.GetErrorOffset(),
                rapidjson::GetParseError_En(reader.GetParseErrorCode())
            ));
        }
    } else if (stream.Tell() != input.size()) {
        throw ParseException("Unexpected bytes after JSON document");
    }
    MaskedJson result{std::string{output.GetString(), std::min(output.GetSize(), log_limit)}, handler.IsLimitReached()};
    if (result.limit_reached) {
        utils::text::utf8::TrimTruncatedEnding(result.text);
    }
    return result;
}

}  // namespace formats::json

USERVER_NAMESPACE_END
