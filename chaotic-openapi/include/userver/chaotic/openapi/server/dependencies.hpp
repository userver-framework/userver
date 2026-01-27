#pragma once

/// @file userver/chaotic/openapi/server/dependencies.hpp
/// @brief Repository for chaotic handlers

#include <type_traits>
#include <vector>

#include <userver/compiler/demangle.hpp>
#include <userver/utils/any_storage.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/move_only_function.hpp>

USERVER_NAMESPACE_BEGIN

namespace chaotic::openapi::server::dependencies {

namespace impl {

template <typename T, typename Tag>
constexpr std::string GetTypeName() {
    std::string str{compiler::GetTypeName<T>()};
    if constexpr (!std::is_same_v<Tag, void>) {
        str += fmt::format("({})", compiler::GetTypeName<Tag>());
    }
    return str;
}

}  // namespace impl

class Factories;

/// @brief Wrapper type used to pass multiple values of the same type into @ref ForHandler::ForHandler for unit tests
///
/// ## Usage example
/// @snippet chaotic-openapi/src/chaotic/openapi/server/dependencies_test.cpp WithTag
///
/// @see FactoryTag
template <typename T, typename Tag>
struct WithTag final {
    /// @cond
    T data;
    /// @endcond
};

/// @brief A storage tag is a unique identifier for a handler dependency
///
/// ## Usage example
/// @snippet chaotic-openapi/src/chaotic/openapi/server/dependencies_test.cpp FactoryTag
///
/// ## Dependencies of the same type
///
/// If you want to refer to multiple values of the same type,
/// use the second template parameter as a tag:
/// @snippet chaotic-openapi/src/chaotic/openapi/server/dependencies_test.cpp FactoryTag
///
/// @see Factories
template <typename T, typename Tag = void>
struct FactoryTag final {
    static WithTag<T, Tag> Wrap(T&& value) { return {std::move(value)}; }
};

/// @brief Dependencies repository for a specific handler
template <typename ForHandlerTag>
class ForHandler final {
public:
    /// @cond
    explicit ForHandler(utils::AnyStorage<ForHandlerTag>&& storage);
    /// @endcond

    /// @brief Creates a repository with specific dependencies values for unit tests
    ///
    /// ## Usage example
    /// @snippet chaotic-openapi/src/chaotic/openapi/server/dependencies_test.cpp ExplicitValues
    ///
    /// @see WithTag to pass multiple values of the same type
    template <typename... Args>
    explicit ForHandler(Args&&... args);

    /// @returns Dependency value for a specific tag
    template <typename T, typename Tag>
    T operator[](const FactoryTag<T, Tag>&);

    /// @cond
    template <typename T, typename Tag>
    T operator[](FactoryTag<T, Tag>&&);
    /// @endcond

private:
    template <typename Arg>
    void Register(Arg&& arg);

    template <typename T, typename Tag>
    void Register(WithTag<T, Tag>&& arg);

    template <typename T, typename Tag>
    void Register(const WithTag<T, Tag>& arg);

    utils::AnyStorage<ForHandlerTag> deps_;
};

/// @brief Repository of builders for all dependencies for the service
/// TODO: snippet
class Factories final {
public:
    /// @brief TODO
    template <typename T>
    using FactoryFunc = utils::move_only_function<T()>;

    /// @brief Registers a dependency builder
    ///
    /// @param func Builder that creates a dependency that can be fetched
    ///        from @ref ForHandler using operator[]
    /// @param Tag a tag used to distinguish multiple values of the same type
    /// @param Func builder type
    template <typename Tag = void, typename Func = void>
    void Register(Func func);

    /// @returns a repository for a specific handler
    template <typename ForHandlerTag>
    ForHandler<ForHandlerTag> Make();

    /// @cond
    template <typename T, typename Tag>
    T MakeData();
    /// @endcond

private:
    utils::AnyStorage<Factories> builder_storage_;
};

namespace impl {

template <typename ForHandlerTag>
using Builders = std::vector<utils::move_only_function<void(Factories&, utils::AnyStorage<ForHandlerTag>&)>>;

template <typename ForHandlerTag>
static Builders<ForHandlerTag>& GetBuilders() {
    static Builders<ForHandlerTag> builders;
    return builders;
}

template <typename AnyTag, typename T, typename Tag>
const utils::AnyStorageDataTag<AnyTag, WithTag<T, Tag>> kTag;

template <typename ForHandlerTag, typename T, typename Tag>
extern const int kBuilderRegistrator;

template <typename T, typename Tag, typename ForHandlerTag>
void BuildAndStore(Factories& f, utils::AnyStorage<ForHandlerTag>& storage)
{
    storage.template Emplace(impl::kTag<ForHandlerTag, T, Tag>, f.MakeData<T, Tag>());
}

template <typename ForHandlerTag, typename T, typename Tag>
const int
    kBuilderRegistrator = (impl::GetBuilders<ForHandlerTag>().emplace_back(BuildAndStore<T, Tag, ForHandlerTag>), 1);

}  // namespace impl

template <typename ForHandlerTag>
ForHandler<ForHandlerTag>::ForHandler(utils::AnyStorage<ForHandlerTag>&& storage)
    : deps_(std::move(storage))
{}

template <typename ForHandlerTag>
template <typename T, typename Tag>
T ForHandler<ForHandlerTag>::operator[](const FactoryTag<T, Tag>&)
{
    (void)impl::kBuilderRegistrator<ForHandlerTag, T, Tag>;

    auto* dep = deps_.template GetOptional(impl::kTag<ForHandlerTag, T, Tag>);

    // Must be non-NULL if this was built from Factories
    // due to Factories::Make implementation
    UINVARIANT(
        dep,
        fmt::format(
            "Trying to access non-registered dependency of type {} from ForHandler<{}>.",
            impl::GetTypeName<T, Tag>(),
            compiler::GetTypeName<ForHandlerTag>()
        )
    );

    return dep->data;
}

template <typename ForHandlerTag>
template <typename T, typename Tag>
T ForHandler<ForHandlerTag>::operator[](FactoryTag<T, Tag>&&)
{
    static_assert(
        !sizeof(T),
        "Seems you're trying to call 'handler[FactoryTag<T>()]' with a temporary value of 'FactoryTag'. Define a "
        "global variable of type 'FactoryTag<T>' (or 'FactoryTag<T, Tag>') and use it insead."
    );
}

template <typename ForHandlerTag>
template <typename... Args>
ForHandler<ForHandlerTag>::ForHandler(Args&&... args)
{
    (Register(std::forward<Args>(args)), ...);
}

template <typename ForHandlerTag>
template <typename Arg>
void ForHandler<ForHandlerTag>::Register(Arg&& arg)
{
    deps_.Emplace(impl::kTag<ForHandlerTag, Arg, void>, std::forward<Arg>(arg));
}

template <typename ForHandlerTag>
template <typename T, typename Tag>
void ForHandler<ForHandlerTag>::Register(WithTag<T, Tag>&& arg)
{
    deps_.Emplace(impl::kTag<ForHandlerTag, T, Tag>, std::move(arg.data));
}

template <typename ForHandlerTag>
template <typename T, typename Tag>
void ForHandler<ForHandlerTag>::Register(const WithTag<T, Tag>&)
{
    static_assert(!sizeof(T), "Calling 'ForHandler::Register()' with non-temporary 'WithTag' argument is forbidden");
}

template <typename Tag, typename Func>
void Factories::Register(Func func)
{
    using T = std::invoke_result_t<Func>;
    FactoryFunc<T> wrapper = std::move(func);

    builder_storage_.template Emplace(impl::kTag<Factories, FactoryFunc<T>, Tag>, std::move(wrapper));
}

template <typename ForHandlerTag>
ForHandler<ForHandlerTag> Factories::Make() {
    utils::AnyStorage<ForHandlerTag> storage;
    for (auto& builder : impl::GetBuilders<ForHandlerTag>()) {
        builder(*this, storage);
    }
    return ForHandler<ForHandlerTag>{std::move(storage)};
}

template <typename T, typename Tag>
T Factories::MakeData()
{
    auto* builder = builder_storage_.template GetOptional(impl::kTag<Factories, FactoryFunc<T>, Tag>);

    UINVARIANT(
        builder,
        fmt::format(
            "Trying to build type {} from Factories, but the builder is not registered. Forgot to call "
            "Factories::Register<{}>(...)?",
            impl::GetTypeName<T, Tag>(),
            compiler::GetTypeName<T>()
        )
    );

    return builder->data();
}

}  // namespace chaotic::openapi::server::dependencies

USERVER_NAMESPACE_END
