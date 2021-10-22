#include "child_process_map.hpp"

#include <unordered_map>

USERVER_NAMESPACE_BEGIN

namespace engine::ev {

using ChildProcessMap = std::unordered_map<int, ChildProcessMapValue>;

namespace {

ChildProcessMap& GetChildProcessMap() {
  static ChildProcessMap child_process_map;
  return child_process_map;
}

}  // namespace

ChildProcessMapValue* ChildProcessMapGetOptional(int pid) {
  auto& child_process_map = GetChildProcessMap();
  auto it = child_process_map.find(pid);
  if (it == child_process_map.end()) return nullptr;
  return &it->second;
}

void ChildProcessMapErase(int pid) { GetChildProcessMap().erase(pid); }

std::pair<ChildProcessMapValue*, bool> ChildProcessMapSet(
    int pid, ChildProcessMapValue&& value) {
  auto res = GetChildProcessMap().emplace(pid, std::move(value));
  return std::make_pair(&res.first->second, res.second);
}

}  // namespace engine::ev

USERVER_NAMESPACE_END
