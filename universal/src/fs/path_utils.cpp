#include <userver/fs/path_utils.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace fs {

std::string GetLexicallyRelative(std::string_view path, std::string_view dir) {
    UASSERT(dir.size() < path.size());
    UASSERT(path.substr(0, dir.size()) == dir);
    auto rel = path.substr(dir.size());
    return std::string{rel};
}

}  // namespace fs

USERVER_NAMESPACE_END
