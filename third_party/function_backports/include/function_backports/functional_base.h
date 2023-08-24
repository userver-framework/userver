#ifndef INCLUDE_STD23____FUNCTIONAL__BASE
#define INCLUDE_STD23____FUNCTIONAL__BASE

#include <functional>
#include <type_traits>
#include <utility>

namespace function_backports
{

template<auto V> struct nontype_t // freestanding
{
  explicit nontype_t() = default;
};

template<auto V> inline constexpr nontype_t<V> nontype{}; // freestanding

using std::in_place_type;
using std::in_place_type_t;
using std::initializer_list;
using std::nullptr_t;

template<class R, class F, class... Args, typename =
    std::enable_if_t<std::is_invocable_r_v<R, F, Args...>>>
constexpr R invoke_r(F &&f, Args &&...args) // freestanding
    noexcept(std::is_nothrow_invocable_r_v<R, F, Args...>)
{
  if constexpr (std::is_void_v<R>)
    std::invoke(std::forward<F>(f), std::forward<Args>(args)...);
  else
    return std::invoke(std::forward<F>(f), std::forward<Args>(args)...);
}

template <typename T>
struct _type_identity {
  using type = T;
};

// See also: https://www.agner.org/optimize/calling_conventions.pdf
template<class T>
inline constexpr auto _select_param_type = []
{
  if constexpr (std::is_trivially_copyable_v<T>)
    return _type_identity<T>();
  else
    return std::add_rvalue_reference<T>();
};

template<class T>
using _param_t =
    typename std::invoke_result_t<decltype(_select_param_type<T>)>::type;

template <class T>
using _remove_cvref_t = std::remove_cv_t<std::remove_reference_t<T>>;

template<class T, class Self>
inline constexpr bool _is_not_self =
    not std::is_same_v<_remove_cvref_t<T>, Self>;

template<class T, template<class...> class>
inline constexpr bool _looks_nullable_to_impl = std::is_member_pointer_v<T>;

template<class F, template<class...> class Self>
inline constexpr bool _looks_nullable_to_impl<F *, Self> =
    std::is_function_v<F>;

template<class... S, template<class...> class Self>
inline constexpr bool _looks_nullable_to_impl<Self<S...>, Self> = true;

template<class S, template<class> class Self>
inline constexpr bool _looks_nullable_to =
    _looks_nullable_to_impl<_remove_cvref_t<S>, Self>;

template<class T> inline constexpr bool _is_not_nontype_t = true;
template<auto f> inline constexpr bool _is_not_nontype_t<nontype_t<f>> = false;

template<class T, class Enable = void> struct _adapt_signature;

template<class F>
struct _adapt_signature<F *, std::enable_if_t<std::is_function_v<F>>>
{
  using type = F;
};

template<class Fp> using _adapt_signature_t =
    typename _adapt_signature<Fp>::type;

template<class S> struct _not_qualifying_this
{};

template<class R, class... Args> struct _not_qualifying_this<R(Args...)>
{
  using type = R(Args...);
};

template<class R, class... Args>
struct _not_qualifying_this<R(Args...) noexcept>
{
  using type = R(Args...) noexcept;
};

template<class R, class... Args>
struct _not_qualifying_this<R(Args...) const> : _not_qualifying_this<R(Args...)>
{};

template<class R, class... Args>
struct _not_qualifying_this<R(Args...) volatile>
    : _not_qualifying_this<R(Args...)>
{};

template<class R, class... Args>
struct _not_qualifying_this<R(Args...) const volatile>
    : _not_qualifying_this<R(Args...)>
{};

template<class R, class... Args>
struct _not_qualifying_this<R(Args...) &> : _not_qualifying_this<R(Args...)>
{};

template<class R, class... Args>
struct _not_qualifying_this<R(Args...) const &>
    : _not_qualifying_this<R(Args...)>
{};

template<class R, class... Args>
struct _not_qualifying_this<R(Args...) volatile &>
    : _not_qualifying_this<R(Args...)>
{};

template<class R, class... Args>
struct _not_qualifying_this<R(Args...) const volatile &>
    : _not_qualifying_this<R(Args...)>
{};

template<class R, class... Args>
struct _not_qualifying_this<R(Args...) &&> : _not_qualifying_this<R(Args...)>
{};

template<class R, class... Args>
struct _not_qualifying_this<R(Args...) const &&>
    : _not_qualifying_this<R(Args...)>
{};

template<class R, class... Args>
struct _not_qualifying_this<R(Args...) volatile &&>
    : _not_qualifying_this<R(Args...)>
{};

template<class R, class... Args>
struct _not_qualifying_this<R(Args...) const volatile &&>
    : _not_qualifying_this<R(Args...)>
{};

template<class R, class... Args>
struct _not_qualifying_this<R(Args...) const noexcept>
    : _not_qualifying_this<R(Args...) noexcept>
{};

template<class R, class... Args>
struct _not_qualifying_this<R(Args...) volatile noexcept>
    : _not_qualifying_this<R(Args...) noexcept>
{};

template<class R, class... Args>
struct _not_qualifying_this<R(Args...) const volatile noexcept>
    : _not_qualifying_this<R(Args...) noexcept>
{};

template<class R, class... Args>
struct _not_qualifying_this<R(Args...) & noexcept>
    : _not_qualifying_this<R(Args...) noexcept>
{};

template<class R, class... Args>
struct _not_qualifying_this<R(Args...) const & noexcept>
    : _not_qualifying_this<R(Args...) noexcept>
{};

template<class R, class... Args>
struct _not_qualifying_this<R(Args...) volatile & noexcept>
    : _not_qualifying_this<R(Args...) noexcept>
{};

template<class R, class... Args>
struct _not_qualifying_this<R(Args...) const volatile & noexcept>
    : _not_qualifying_this<R(Args...) noexcept>
{};

template<class R, class... Args>
struct _not_qualifying_this<R(Args...) && noexcept>
    : _not_qualifying_this<R(Args...) noexcept>
{};

template<class R, class... Args>
struct _not_qualifying_this<R(Args...) const && noexcept>
    : _not_qualifying_this<R(Args...) noexcept>
{};

template<class R, class... Args>
struct _not_qualifying_this<R(Args...) volatile && noexcept>
    : _not_qualifying_this<R(Args...) noexcept>
{};

template<class R, class... Args>
struct _not_qualifying_this<R(Args...) const volatile && noexcept>
    : _not_qualifying_this<R(Args...) noexcept>
{};

template<class F, class T, typename Enable = void>
struct _drop_first_arg_to_invoke;

template<class T, class R, class G, class... Args>
struct _drop_first_arg_to_invoke<R (*)(G, Args...), T, void>
{
  using type = R(Args...);
};

template<class T, class R, class G, class... Args>
struct _drop_first_arg_to_invoke<R (*)(G, Args...) noexcept, T, void>
{
  using type = R(Args...) noexcept;
};

template<class T, class M, class G>
struct _drop_first_arg_to_invoke<M G::*, T,
                                 std::enable_if_t<std::is_object_v<M>>>
{
  using type = std::invoke_result_t<M G::*, T>();
};

template<class T, class M, class G>
struct _drop_first_arg_to_invoke<M G::*, T,
                                 std::enable_if_t<std::is_function_v<M>>>
    : _not_qualifying_this<M>
{};

template<class F, class T>
using _drop_first_arg_to_invoke_t =
    typename _drop_first_arg_to_invoke<F, T>::type;

} // namespace function_backports

#endif
