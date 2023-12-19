#ifndef INCLUDE_STD23_FUNCTION__REF
#define INCLUDE_STD23_FUNCTION__REF

#include "functional_base.h"

#include <cassert>

namespace function_backports
{

template<class Sig> struct _qual_fn_sig;

template<class R, class... Args> struct _qual_fn_sig<R(Args...)>
{
  using function = R(Args...);
  static constexpr bool is_noexcept = false;

  template<class... T>
  static constexpr bool is_invocable_using =
      std::is_invocable_r_v<R, T..., Args...>;

  template<class T> using cv = T;
};

template<class R, class... Args> struct _qual_fn_sig<R(Args...) noexcept>
{
  using function = R(Args...);
  static constexpr bool is_noexcept = true;

  template<class... T>
  static constexpr bool is_invocable_using =
      std::is_nothrow_invocable_r_v<R, T..., Args...>;

  template<class T> using cv = T;
};

template<class R, class... Args>
struct _qual_fn_sig<R(Args...) const> : _qual_fn_sig<R(Args...)>
{
  template<class T> using cv = T const;
};

template<class R, class... Args>
struct _qual_fn_sig<R(Args...) const noexcept>
    : _qual_fn_sig<R(Args...) noexcept>
{
  template<class T> using cv = T const;
};

struct _function_ref_base
{
  union storage
  {
    void *p_ = nullptr;
    void const *cp_;
    void (*fp_)();

    constexpr storage() noexcept = default;

    template <class T, class = std::enable_if_t<std::is_object_v<T> ||
                                                std::is_function_v<T>>>
    constexpr explicit storage(T *p) noexcept
        : storage(p, std::bool_constant<std::is_object_v<T>>{}) {}

    template<class T, class = std::enable_if_t<std::is_object_v<T>>>
    constexpr explicit storage(T const *p) noexcept : cp_(p) {}

   private:
    template<class T>
    constexpr explicit storage(T *p, std::true_type) noexcept : p_(p) {}

    template<class T>
    constexpr explicit storage(T *p, std::false_type) noexcept
        : fp_(reinterpret_cast<decltype(fp_)>(p)) {}
  };

  template<class T> constexpr static auto get(storage obj)
  {
    if constexpr (std::is_const_v<T>)
      return static_cast<T *>(obj.cp_);
    else if constexpr (std::is_object_v<T>)
      return static_cast<T *>(obj.p_);
    else
      return reinterpret_cast<T *>(obj.fp_);
  }
};

template<class Sig, class = typename _qual_fn_sig<Sig>::function>
class function_ref; // freestanding

template<class Sig, class R, class... Args>
class function_ref<Sig, R(Args...)> // freestanding
    : _function_ref_base
{
  using signature = _qual_fn_sig<Sig>;

  template<class T> using cv = typename signature::template cv<T>;
  template<class T> using cvref = cv<T> &;
  static constexpr bool noex = signature::is_noexcept;

  template<class... T>
  static constexpr bool is_invocable_using =
      signature::template is_invocable_using<T...>;

  typedef R fwd_t(storage, _param_t<Args>...) noexcept(noex);
  fwd_t *fptr_ = nullptr;
  storage obj_;

 public:
  template <class F, class = std::enable_if_t<std::is_function_v<F> and
                                              is_invocable_using<F>>>
  function_ref(F *f) noexcept
      : fptr_(
            [](storage fn_, _param_t<Args>... args) noexcept(noex) -> R
            {
              if constexpr (std::is_void_v<R>)
                get<F>(fn_)(static_cast<decltype(args)>(args)...);
              else
                return get<F>(fn_)(static_cast<decltype(args)>(args)...);
            }),
        obj_(f)
  {
    assert(f != nullptr && "must reference a function");
  }

  template <class F, class T = std::remove_reference_t<F>,
            class = std::enable_if_t<_is_not_self<F, function_ref> and
                                     not std::is_member_pointer_v<T> and
                                     is_invocable_using<cvref<T>>>>
  constexpr function_ref(F &&f) noexcept
      : fptr_(
            [](storage fn_, _param_t<Args>... args) noexcept(noex) -> R
            {
              cvref<T> obj = *get<T>(fn_);
              if constexpr (std::is_void_v<R>)
                obj(static_cast<decltype(args)>(args)...);
              else
                return obj(static_cast<decltype(args)>(args)...);
            }),
        obj_(std::addressof(f))
  {}

  template <class T, typename = std::enable_if_t<
                         _is_not_self<T, function_ref> and
                         not std::is_pointer_v<T> and _is_not_nontype_t<T>>>
  function_ref &operator=(T)
  = delete;

  template<auto f, typename = std::enable_if_t<is_invocable_using<decltype(f)>>>
  constexpr function_ref(nontype_t<f>) noexcept
      : fptr_(
            [](storage, _param_t<Args>... args) noexcept(noex) -> R {
              return function_backports::invoke_r<R>(
                  f, static_cast<decltype(args)>(args)...);
            })
  {
    using F = decltype(f);
    if constexpr (std::is_pointer_v<F> or std::is_member_pointer_v<F>)
      static_assert(f != nullptr, "NTTP callable must be usable");
  }

  template <
      auto f, class U, class T = std::remove_reference_t<U>,
      typename = std::enable_if_t<not std::is_rvalue_reference_v<U&&> and
                                  is_invocable_using<decltype(f), cvref<T>>>>
  constexpr function_ref(nontype_t<f>, U&& obj) noexcept
      : fptr_(
            [](storage this_, _param_t<Args>... args) noexcept(noex) -> R
            {
              cvref<T> obj = *get<T>(this_);
              return function_backports::invoke_r<R>(
                  f, obj, static_cast<decltype(args)>(args)...);
            }),
        obj_(std::addressof(obj))
  {
    using F = decltype(f);
    if constexpr (std::is_pointer_v<F> or std::is_member_pointer_v<F>)
      static_assert(f != nullptr, "NTTP callable must be usable");
  }

  template <auto f, class T,
            class = std::enable_if_t<is_invocable_using<decltype(f), cv<T>*>>>
  constexpr function_ref(nontype_t<f>, cv<T>* obj) noexcept
      : fptr_(
            [](storage this_, _param_t<Args>... args) noexcept(noex) -> R
            {
              return function_backports::invoke_r<R>(
                  f, get<cv<T>>(this_),
                  static_cast<decltype(args)>(args)...);
            }),
        obj_(obj)
  {
    using F = decltype(f);
    if constexpr (std::is_pointer_v<F> or std::is_member_pointer_v<F>)
      static_assert(f != nullptr, "NTTP callable must be usable");

    if constexpr (std::is_member_pointer_v<F>)
      assert(obj != nullptr && "must reference an object");
  }

  constexpr R operator()(Args... args) const noexcept(noex)
  {
    return fptr_(obj_, std::forward<Args>(args)...);
  }
};

template<class F, class = std::enable_if_t<std::is_function_v<F>>>
function_ref(F *) -> function_ref<F>;

template<auto V>
function_ref(nontype_t<V>) -> function_ref<_adapt_signature_t<decltype(V)>>;

template<auto V, class T>
function_ref(nontype_t<V>, T &&)
    -> function_ref<_drop_first_arg_to_invoke_t<decltype(V), T &>>;

} // namespace function_backports

#endif
