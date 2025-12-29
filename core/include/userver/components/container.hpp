#pragma once

/// @file
/// @brief @copybrief components::Container

#include <userver/components/component_base.hpp>
#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/formats/common/meta.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

template <typename T>
struct Of {};

/// @cond
constexpr std::string_view ContainerName(Of<void>) { return {}; }
/// @endcond

template <typename T>
class Container;

namespace impl {

template <typename T, typename = void>
struct ContainerHasName : std::false_type {};

template <typename T>
struct ContainerHasName<T, utils::void_t<decltype(ContainerName(Of<T>()))>> : std::true_type {};

template <typename T>
constexpr std::string_view GetContainerName()
{
    if constexpr (ContainerHasName<T>::value) {
        return ContainerName(Of<T>());
    } else {
        static_assert(!sizeof(T), "Container name is not registered. Forgot to define ContainerName(Of<T>)?");
        return {};
    }
}

template <typename U>
struct DependencyLocator {
    const ComponentConfig& config;
    const ComponentContext& context;

    template <typename T>
    using LocateDependencyResult = decltype(LocateDependency(
        WithType<std::decay_t<T>>{},
        std::declval<const ComponentConfig&>(),
        std::declval<const ComponentContext&>()
    ));

    template <typename T>
    static constexpr bool kIsReference = std::is_reference_v<LocateDependencyResult<T>>;

    template <typename T>
    static constexpr bool kIsLocatable =
        !std::is_same_v<std::decay_t<T>, std::decay_t<U>>  // 1) skip U::U(const U&)
        && !!sizeof(LocateDependencyResult<T>)             // 2) U is locate'able
        ;

    template <typename T, typename = std::enable_if_t<!kIsReference<T> && kIsLocatable<T>>>
    operator T() const {
        return LocateDependency(WithType<std::decay_t<T>>{}, config, context);
    }

    template <typename T, typename = std::enable_if_t<kIsReference<T> && kIsLocatable<T>>>
    operator T&() const {
        return LocateDependency(WithType<std::decay_t<T>>{}, config, context);
    }
};

}  // namespace impl

/// @brief A function that extracts an in-component-stored data
/// (probably, reference) of type `T`, `T&`, or `const T&` from
/// @ref components::ComponentContext.
///
/// You may define your own version of `LocateDependency` for a custom type `T`
/// either in T's namespace or (if T's namespace is not extendable for some
/// reason) in `USERVER_NAMESPACE::components` namespace. Usually it is defined
/// for a component `C` that returns `T` from `C::GetSomeT()` as following:
///
/// @snippet core/src/dynamic_config/storage/component.cpp LocateDependency example
template <typename T>
std::enable_if_t<formats::common::impl::kHasParse<yaml_config::YamlConfig, T> && std::is_class_v<T>, T>
LocateDependency(WithType<T>, const ComponentConfig& config, const ComponentContext&)
{
    return config.As<T>();
}

template <typename T>
std::enable_if_t<impl::ContainerHasName<T>::value, T&> LocateDependency(
    WithType<T>,
    const ComponentConfig&,
    const ComponentContext& context
)
{
    return context.FindComponent<Container<T>>().Get();
}

template <typename T>
std::enable_if_t<std::is_base_of_v<RawComponentBase, T>, T&> LocateDependency(
    WithType<T>,
    const ComponentConfig&,
    const ComponentContext& context
)
{
    return context.FindComponent<T>();
}

/// @brief A simple Component that creates, hold, and distributes
/// an object of user type `T`. The component has a name equal to the constexpr
/// std::string_view result of user-defined `ContainerName(Of<T>{})`.
///
/// Every dependency of type `X` is resolved by the component using
/// @ref components::LocateDependency. By default, it is able to resolve
/// the following types of dependencies:
/// - `X` that was previously registered via defining `ContainerName(Of<X>)`. That is
///   a containerized dependency.
/// - `X` that declares `Parse(const yaml_config::YamlConfig& value, formats::parse::To<X>)`.
///   This is a static config dependency.
///
/// Besides that, you may define our own specialization for
/// @ref components::LocateDependency to allow fetching dependencies
/// from non-container components. E.g. userver already defines
/// @ref dynamic_config::Source from @ref components::DynamicConfig.
///
/// The core limitation of a type `T` registered via `ContainerName(Of<T>)` is
/// that it is not able to explicitly use
/// @ref components::ComponentContext in the consturctor.
/// But if you want to only "fetch" something from the context,
/// you're always able to define your own @ref components::LocateDependency
/// that fetches everything you need from @ref components::ComponentContext.
///
/// Example:
///
/// @snippet core/src/components/container_test.cpp definition
/// @snippet core/src/components/container_test.cpp registration
template <typename T>
class Container final : public ComponentBase {
public:
    Container(const ComponentConfig& config, const ComponentContext& context)
        : ComponentBase(config, context),
          content_(Build(config, context))
    {}

    T& Get() { return content_; }

    static constexpr auto kConfigFileMode = ConfigFileMode::kNotRequired;

    static constexpr std::string_view kName = impl::GetContainerName<T>();

private:
    static T Build(const ComponentConfig& config, const ComponentContext& context)
    {
        using Arg = impl::DependencyLocator<T>;
        Arg arg{config, context};

        // A kind of copy-paste, but a more generic solution would be too template-ish
        // and absolutely non-readable :(
        if constexpr (std::is_constructible<T>::value) {
            return T();
        } else if constexpr (std::is_constructible<T, Arg>::value) {
            return T(arg);
        } else if constexpr (std::is_constructible<T, Arg, Arg>::value) {
            return T(arg, arg);
        } else if constexpr (std::is_constructible<T, Arg, Arg, Arg>::value) {
            return T(arg, arg, arg);
        } else if constexpr (std::is_constructible<T, Arg, Arg, Arg, Arg>::value) {
            return T(arg, arg, arg, arg);
        } else if constexpr (std::is_constructible<T, Arg, Arg, Arg, Arg, Arg>::value) {
            return T(arg, arg, arg, arg, arg);
        } else if constexpr (std::is_constructible<T, Arg, Arg, Arg, Arg, Arg, Arg>::value) {
            return T(arg, arg, arg, arg, arg, arg);
        } else if constexpr (std::is_constructible<T, Arg, Arg, Arg, Arg, Arg, Arg, Arg>::value) {
            return T(arg, arg, arg, arg, arg, arg, arg);
        } else if constexpr (std::is_constructible<T, Arg, Arg, Arg, Arg, Arg, Arg, Arg, Arg>::value) {
            return T(arg, arg, arg, arg, arg, arg, arg, arg);
        } else if constexpr (std::is_constructible<T, Arg, Arg, Arg, Arg, Arg, Arg, Arg, Arg, Arg>::value) {
            return T(arg, arg, arg, arg, arg, arg, arg, arg, arg);
        } else if constexpr (std::is_constructible<T, Arg, Arg, Arg, Arg, Arg, Arg, Arg, Arg, Arg, Arg>::value) {
            return T(arg, arg, arg, arg, arg, arg, arg, arg, arg, arg);
        } else {
            static_assert(
                !sizeof(T),
                "Failed to find an appropriate version of T::T(...). Please check that T has a constructor with "
                "arguments of containerized types or locatable via LocateDependency()."
            );
        }
    }

    T content_;
};

}  // namespace components

USERVER_NAMESPACE_END
