#include <userver/formats/common/utils.hpp>

#include <userver/utils/text_light.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::common {

std::vector<std::string> SplitPathString(std::string_view path) {
    if (path.empty()) {
        return {};
    }
    return utils::text::Split(path, ".", utils::text::SplitFlags::kNone);
}

}  // namespace formats::common

USERVER_NAMESPACE_END
