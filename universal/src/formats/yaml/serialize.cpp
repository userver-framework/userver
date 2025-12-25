#include <userver/formats/yaml/serialize.hpp>

#include <algorithm>
#include <array>
#include <fstream>
#include <functional>
#include <memory>
#include <sstream>
#include <string_view>
#include <type_traits>

#include <fmt/format.h>
#include <yaml-cpp/yaml.h>
#include <boost/container/small_vector.hpp>

#include <userver/formats/common/path.hpp>
#include <userver/formats/yaml/exception.hpp>
#include <userver/formats/yaml/value.hpp>
#include <userver/utils/enumerate.hpp>
#include <userver/utils/not_null.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::yaml {

namespace {

constexpr std::size_t kInitialStackDepth = 16;
constexpr std::size_t kInitialSortedKeysSize = 32;

struct PathElement {
    explicit PathElement(YAML::Node& parent)
        : current(parent.begin()),
          end(parent.end())
    {}

    // Cannot store Node[&], because it has broken copy/move semantics for iteration.
    // https://github.com/jbeder/yaml-cpp/issues/928
    //
    // Cannot store const_iterator, because incurs extra copies.
    YAML::Node::iterator current;
    YAML::Node::iterator end;
};

class RawYamlVisitor final {
public:
    explicit RawYamlVisitor(YAML::Node& root)
        : root_(root)
    {}

    template <typename Visitor, typename = std::enable_if_t<std::is_invocable_v<Visitor&, YAML::Node&>>>
    void VisitPreOrder(Visitor visitor) {
        if (!root_.IsSequence() && !root_.IsMap()) {
            return;
        }

        visitor(root_);
        path_stack_.emplace_back(root_);

        while (true) {
            if (path_stack_.back().current == path_stack_.back().end) {
                path_stack_.pop_back();
                if (path_stack_.empty()) {
                    break;
                }
                ++path_stack_.back().current;
                continue;
            }

            auto item = *path_stack_.back().current;
            auto& node = item.IsDefined() ? item : item.second;

            visitor(node);
            if (node.IsSequence() || node.IsMap()) {
                path_stack_.emplace_back(node);
            } else {
                ++path_stack_.back().current;
            }
        }
    }

    // Complexity: O(whole-yaml-size)
    std::string ComputeCurrentPath() const {
        if (path_stack_.empty()) {
            return common::kPathRoot;
        }

        std::string path;
        for (const auto [idx, element] : utils::enumerate(path_stack_)) {
            if (element.current->IsDefined()) {
                // Sequence iterator.
                const auto begin = idx == 0 ? root_.begin() : path_stack_[idx - 1].current->begin();
                common::AppendPath(path, std::distance(begin, element.current));
            } else {
                // Map iterator.
                const auto map_item = *element.current;
                const auto& key = map_item.first;
                switch (key.Type()) {
                    case YAML::NodeType::Null:
                        common::AppendPath(path, "null");
                        break;
                    case YAML::NodeType::Scalar:
                        common::AppendPath(path, key.Scalar());
                        break;
                    default: {
                        // YAML can have non-scalar keys:
                        // https://yaml.org/spec/1.2.2/#mapping
                        std::ostringstream stream;
                        stream << key;
                        common::AppendPath(path, stream.str());
                        break;
                    }
                }
            }
        }
        return path;
    }

private:
    YAML::Node& root_;
    boost::container::small_vector<PathElement, kInitialStackDepth> path_stack_;
};

// Non-unique keys in mappings are not allowed, but yaml-cpp does not check for them:
// https://github.com/jbeder/yaml-cpp/issues/60
class UniquenessChecker final {
public:
    explicit UniquenessChecker(YAML::Node& root)
        : visitor_(root)
    {}

    void CheckKeyUniquenessIterative() {
        visitor_.VisitPreOrder([this](const YAML::Node& node) {
            if (node.IsMap()) {
                CheckSingleMapKeyUniqueness(node);
            }
        });
    }

private:
    void CheckSingleMapKeyUniqueness(const YAML::Node& node) {
        UASSERT(node.IsMap());
        sorted_keys_.clear();
        sorted_keys_.reserve(node.size());
        for (const auto item : node) {
            const auto& key = item.first;
            switch (key.Type()) {
                case YAML::NodeType::Null:
                    sorted_keys_.push_back("null");
                    break;
                case YAML::NodeType::Scalar:
                    sorted_keys_.push_back(key.Scalar());
                    break;
                default:
                    // This is a valid key, but too complex to check. YAML nodes may be recursive and infinitely deep.
                    break;
            }
        }

        std::sort(sorted_keys_.begin(), sorted_keys_.end());
        if (const auto duplicate = std::adjacent_find(sorted_keys_.begin(), sorted_keys_.end());
            duplicate != sorted_keys_.end())
        {
            throw ParseException(
                fmt::format("Duplicate mapping key '{}' at path '{}'", *duplicate, visitor_.ComputeCurrentPath())
            );
        }
    }

    RawYamlVisitor visitor_;
    boost::container::small_vector<std::string_view, kInitialSortedKeysSize> sorted_keys_;
};

}  // namespace

formats::yaml::Value FromString(const std::string& doc) {
    if (doc.empty()) {
        throw ParseException("YAML document is empty");
    }

    try {
        auto node = YAML::Load(doc);
        UniquenessChecker{node}.CheckKeyUniquenessIterative();
        return formats::yaml::Value(node);
    } catch (const YAML::ParserException& e) {
        throw ParseException(e.what());
    }
}

formats::yaml::Value FromStream(std::istream& is) {
    if (!is) {
        throw BadStreamException(is);
    }

    try {
        auto node = YAML::Load(is);
        UniquenessChecker{node}.CheckKeyUniquenessIterative();
        return formats::yaml::Value(node);
    } catch (const YAML::ParserException& e) {
        throw ParseException(e.what());
    }
}

void Serialize(const formats::yaml::Value& doc, std::ostream& os) {
    os << doc.GetNative();
    if (!os) {
        throw BadStreamException(os);
    }
}

std::string ToString(const formats::yaml::Value& doc) {
    std::ostringstream os;
    Serialize(doc, os);
    return os.str();
}

namespace blocking {
formats::yaml::Value FromFile(const std::string& path) {
    std::ifstream is(path);
    try {
        return FromStream(is);
    } catch (const std::exception& e) {
        throw ParseException(fmt::format("Parsing '{}' failed. {}", path, e.what()));
    }
}
}  // namespace blocking

namespace impl {

formats::yaml::Value FromStringAllowRepeatedKeys(const std::string& doc) {
    if (doc.empty()) {
        throw ParseException("YAML document is empty");
    }

    try {
        return formats::yaml::Value(YAML::Load(doc));
    } catch (const YAML::ParserException& e) {
        throw ParseException(e.what());
    }
}

}  // namespace impl

}  // namespace formats::yaml

USERVER_NAMESPACE_END
