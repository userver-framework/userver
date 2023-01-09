#include <userver/fs/temp_file.hpp>

#include <userver/utils/async.hpp>

USERVER_NAMESPACE_BEGIN

namespace fs {

TempFile TempFile::Create(engine::TaskProcessor& fs_task_processor) {
  return {fs_task_processor, blocking::TempFile::Create()};
}

TempFile TempFile::Create(std::string_view parent_path,
                          std::string_view name_prefix,
                          engine::TaskProcessor& fs_task_processor) {
  return {fs_task_processor,
          blocking::TempFile::Create(parent_path, name_prefix)};
}

TempFile::~TempFile() { std::move(*this).Remove(); }

TempFile TempFile::Adopt(std::string path,
                         engine::TaskProcessor& fs_task_processor) {
  return {fs_task_processor, blocking::TempFile::Adopt(path)};
}

const std::string& TempFile::GetPath() const { return temp_file_.GetPath(); }

void TempFile::Remove() && {
  utils::Async(fs_task_processor_, "temp_file_remove", [this] {
    std::move(temp_file_).Remove();
  }).Get();
}

}  // namespace fs

USERVER_NAMESPACE_END
