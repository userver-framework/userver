#include <userver/formats/json/impl/mutable_value_wrapper.hpp>

#include <optional>
#include <utility>
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
    std::shared_ptr<const JsonPath> parent;

    template <typename T>
    JsonPath(T&& element, std::shared_ptr<const JsonPath> parent)
        : element(std::forward<T>(element)),
          parent(std::move(parent))
    {}

    static Value& FetchMember(Value& root, const JsonPath& path);
    static std::string ToString(const JsonPath&);
};

struct MutableValueWrapper::Impl {
    Impl() = default;

    Impl(VersionedValuePtr&& root, size_t version)
        : value(std::move(root)),
          current_version(version)
    {}

    Impl(JsonPath&& path, VersionedValuePtr root, const Value& member, int depth, size_t version)
        : path(std::move(path)),
          value(MakeValue(std::move(root), member, depth)),
          current_version(version)
    {}

    static formats::json::Value MakeValue(VersionedValuePtr&& root, const Value& member, int depth) {
        // Custom root_ptr_for_path_ is not used for Value inside ValueBuilder.
        auto* root_ptr_for_path = root.Get();
        return {formats::json::Value::EmplaceEnabler{}, std::move(root), root_ptr_for_path, &member, depth};
    }

    const JsonPath* GetPath() const { return path ? &*path : shared_path.get(); }

    mutable std::optional<JsonPath> path;
    mutable std::shared_ptr<const JsonPath> shared_path;
    mutable formats::json::Value value;
    mutable size_t current_version{VersionedValuePtr::kInvalidVersion};
};

MutableValueWrapper::MutableValueWrapper() = default;

MutableValueWrapper::MutableValueWrapper(VersionedValuePtr root)
    : impl_(std::move(root), root.Version())
{}

MutableValueWrapper::MutableValueWrapper(JsonPath path, VersionedValuePtr root, const Value& member, int depth)
    : impl_(std::move(path), std::move(root), member, depth, root.Version())
{}

MutableValueWrapper::MutableValueWrapper(const MutableValueWrapper&) = default;

MutableValueWrapper::MutableValueWrapper(MutableValueWrapper&&) noexcept = default;

MutableValueWrapper& MutableValueWrapper::operator=(const MutableValueWrapper&) = default;

MutableValueWrapper& MutableValueWrapper::operator=(MutableValueWrapper&&) noexcept = default;

MutableValueWrapper::~MutableValueWrapper() = default;

MutableValueWrapper MutableValueWrapper::WrapMember(std::string&& element, const Value& member) const {
    EnsureCurrent();
    return {JsonPath{std::move(element), SharePath()}, impl_->value.holder_, member, impl_->value.depth_ + 1};
}

MutableValueWrapper MutableValueWrapper::WrapElement(size_t index) const {
    EnsureCurrent();
    return {
        JsonPath{index, SharePath()},
        impl_->value.holder_,
        (*impl_->value.value_ptr_)[static_cast<::rapidjson::SizeType>(index)],
        impl_->value.depth_ + 1
    };
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
    const auto* path = impl_->GetPath();
    return path ? JsonPath::ToString(*path) : common::kPathRoot;
}

bool MutableValueWrapper::IsRoot() const { return !impl_->path && !impl_->shared_path; }

bool MutableValueWrapper::IsUniqueRoot() const {
    return IsRoot() && (impl_->value.holder_.IsUnique() || impl_->value.holder_.IsSharedNull());
}

std::shared_ptr<const MutableValueWrapper::JsonPath> MutableValueWrapper::SharePath() const {
    if (impl_->path) {
        UASSERT(!impl_->shared_path);
        impl_->shared_path = std::make_shared<const JsonPath>(std::move(*impl_->path));
        impl_->path.reset();
    }
    return impl_->shared_path;
}

formats::json::Value MutableValueWrapper::ExtractValue() && {
    UASSERT(IsRoot());
    auto result = std::exchange(impl_->value, formats::json::Value{});
    impl_->current_version = impl_->value.holder_.Version();
    return result;
}

void MutableValueWrapper::OnMembersChange() {
    UASSERT(impl_->value.holder_.Version() == impl_->current_version);
    impl_->value.holder_.BumpVersion();
    impl_->current_version = impl_->value.holder_.Version();
}

void MutableValueWrapper::EnsureCurrent() const {
    if (impl_->value.holder_.Version() == impl_->current_version) [[likely]] {
        return;
    }
    // only refetch if we had a value
    if (impl_->current_version != VersionedValuePtr::kInvalidVersion) {
        const auto* path = impl_->GetPath();
        impl_->value.SetNative(path ? JsonPath::FetchMember(*impl_->value.holder_, *path) : *impl_->value.holder_);
    }
    impl_->current_version = impl_->value.holder_.Version();
}

Value& MutableValueWrapper::JsonPath::FetchMember(Value& root, const JsonPath& path) {
    class Visitor {
    public:
        Visitor(Value& value, const JsonPath& path)
            : value_(value),
              path_(path)
        {}

        Value& operator()(const std::string& element) {
            if (!value_.IsObject()) {
                throw TypeMismatchException(GetExtendedType(value_), Type::kObjectValue, ToString(path_));
            }
            auto it = value_.FindMember(element);
            if (it == value_.MemberEnd()) {
                throw MemberMissingException(ToString(path_));
            }
            return it->value;
        }

        Value& operator()(size_t index) {
            if (!value_.IsArray()) {
                throw TypeMismatchException(GetExtendedType(value_), Type::kArrayValue, ToString(path_));
            }
            if (index >= value_.Size()) {
                throw OutOfBoundsException(index, value_.Size(), ToString(path_));
            }
            return value_[static_cast<::rapidjson::SizeType>(index)];
        }

    private:
        Value& value_;
        const JsonPath& path_;
    };

    auto& parent = path.parent ? FetchMember(root, *path.parent) : root;
    return std::visit(Visitor{parent, path}, path.element);
}

std::string MutableValueWrapper::JsonPath::ToString(const JsonPath& path) {
    auto path_str = path.parent ? ToString(*path.parent) : std::string{};
    std::visit([&path_str](const auto& element) { common::AppendPath(path_str, element); }, path.element);
    return path_str;
}

}  // namespace formats::json::impl

USERVER_NAMESPACE_END
