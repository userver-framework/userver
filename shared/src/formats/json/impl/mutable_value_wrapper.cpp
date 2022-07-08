#include <userver/formats/json/impl/mutable_value_wrapper.hpp>

#include <variant>
#include <vector>

#include <rapidjson/document.h>

#include <userver/formats/json/exception.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/utils/assert.hpp>

#include <formats/json/impl/exttypes.hpp>
#include <userver/formats/common/path.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::json::impl {

struct MutableValueWrapper::JsonPath {
  std::variant<std::string, size_t> element;
  std::shared_ptr<JsonPath> parent;

  template <typename T>
  JsonPath(T&& element_, std::shared_ptr<JsonPath> parent_)
      : element(std::forward<T>(element_)), parent(std::move(parent_)) {}

  static Value& FetchMember(Value& root, const std::shared_ptr<JsonPath>& path);
  static std::string ToString(const std::shared_ptr<JsonPath>&);
};

struct MutableValueWrapper::Impl {
  Impl() noexcept = default;

  Impl(VersionedValuePtr&& root, size_t version)
      : value(std::move(root)), current_version(version) {}

  Impl(std::shared_ptr<JsonPath>&& path, VersionedValuePtr root,
       const Value& member, int depth, size_t version)
      : path(std::move(path)),
        value(std::move(root), &member, depth),
        current_version(version) {}

  std::shared_ptr<JsonPath> path;
  mutable formats::json::Value value;
  mutable size_t current_version{VersionedValuePtr::kInvalidVersion};
};

MutableValueWrapper::MutableValueWrapper() noexcept = default;

MutableValueWrapper::MutableValueWrapper(VersionedValuePtr root)
    : impl_(std::move(root), root.Version()) {}

MutableValueWrapper::MutableValueWrapper(std::shared_ptr<JsonPath> path,
                                         VersionedValuePtr root,
                                         const Value& member, int depth)
    : impl_(std::move(path), std::move(root), member, depth, root.Version()) {}

MutableValueWrapper::MutableValueWrapper(const MutableValueWrapper&) = default;

MutableValueWrapper::MutableValueWrapper(MutableValueWrapper&&) noexcept =
    default;

MutableValueWrapper& MutableValueWrapper::operator=(
    const MutableValueWrapper&) = default;

MutableValueWrapper& MutableValueWrapper::operator=(
    MutableValueWrapper&&) noexcept = default;

MutableValueWrapper::~MutableValueWrapper() = default;

MutableValueWrapper MutableValueWrapper::WrapMember(std::string&& element,
                                                    const Value& member) const {
  EnsureCurrent();
  return {std::make_shared<JsonPath>(std::move(element), impl_->path),
          impl_->value.root_, member, impl_->value.depth_ + 1};
}

MutableValueWrapper MutableValueWrapper::WrapElement(size_t index) const {
  EnsureCurrent();
  return {std::make_shared<JsonPath>(index, impl_->path), impl_->value.root_,
          (*impl_->value.value_ptr_)[static_cast<::rapidjson::SizeType>(index)],
          impl_->value.depth_ + 1};
}

const formats::json::Value& MutableValueWrapper::operator*() const {
  EnsureCurrent();
  return impl_->value;
}

formats::json::Value& MutableValueWrapper::operator*() {
  EnsureCurrent();
  return impl_->value;
}

const formats::json::Value* MutableValueWrapper::operator->() const {
  EnsureCurrent();
  return &impl_->value;
}

formats::json::Value* MutableValueWrapper::operator->() {
  EnsureCurrent();
  return &impl_->value;
}

std::string MutableValueWrapper::GetPath() const {
  return impl_->path ? JsonPath::ToString(impl_->path) : common::kPathRoot;
}

formats::json::Value MutableValueWrapper::ExtractValue() && {
  return std::move(impl_->value);
}

void MutableValueWrapper::OnMembersChange() {
  UASSERT(impl_->value.root_.Version() == impl_->current_version);
  impl_->value.root_.BumpVersion();
  impl_->current_version = impl_->value.root_.Version();
}

void MutableValueWrapper::EnsureCurrent() const {
  if (impl_->value.root_.Version() == impl_->current_version) {
    return;
  }
  // only refetch if we had a value
  if (impl_->current_version != VersionedValuePtr::kInvalidVersion) {
    impl_->value.SetNative(
        JsonPath::FetchMember(*impl_->value.root_, impl_->path));
  }
  impl_->current_version = impl_->value.root_.Version();
}

Value& MutableValueWrapper::JsonPath::FetchMember(
    Value& root, const std::shared_ptr<JsonPath>& path) {
  class Visitor {
   public:
    Visitor(Value& value, const std::shared_ptr<JsonPath>& path)
        : value_(value), path_(path) {}

    Value& operator()(const std::string& element) {
      if (!value_.IsObject()) {
        throw TypeMismatchException(GetExtendedType(value_), Type::objectValue,
                                    ToString(path_));
      }
      auto it = value_.FindMember(element);
      if (it == value_.MemberEnd()) {
        throw MemberMissingException(ToString(path_));
      }
      return it->value;
    }

    Value& operator()(size_t index) {
      if (!value_.IsArray()) {
        throw TypeMismatchException(GetExtendedType(value_), Type::arrayValue,
                                    ToString(path_));
      }
      if (index >= value_.Size()) {
        throw OutOfBoundsException(index, value_.Size(), ToString(path_));
      }
      return value_[static_cast<::rapidjson::SizeType>(index)];
    }

   private:
    Value& value_;
    const std::shared_ptr<JsonPath>& path_;
  };

  if (!path) return root;
  return std::visit(Visitor{FetchMember(root, path->parent), path},
                    path->element);
}

std::string MutableValueWrapper::JsonPath::ToString(
    const std::shared_ptr<JsonPath>& path) {
  if (!path) return {};
  auto path_str = ToString(path->parent);
  std::visit(
      [&path_str](const auto& element) {
        common::AppendPath(path_str, element);
      },
      path->element);
  return path_str;
}

}  // namespace formats::json::impl

USERVER_NAMESPACE_END
