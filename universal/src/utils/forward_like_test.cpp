#include <userver/utils/forward_like.hpp>

#include <type_traits>
#include <utility>

USERVER_NAMESPACE_BEGIN

namespace {
    
template <typename T, typename U>
using ForwardLikeResult = decltype(utils::ForwardLike<T>(std::declval<U>()));

static_assert(std::is_same_v<ForwardLikeResult<int, int>, int&&>);
static_assert(std::is_same_v<ForwardLikeResult<int, const int>, const int&&>);
static_assert(std::is_same_v<ForwardLikeResult<int, volatile int>, volatile int&&>);
static_assert(std::is_same_v<ForwardLikeResult<int, const volatile int>, const volatile int&&>);
static_assert(std::is_same_v<ForwardLikeResult<int, int&>, int&&>);
static_assert(std::is_same_v<ForwardLikeResult<int, const int&>, const int&&>);
static_assert(std::is_same_v<ForwardLikeResult<int, volatile int&>, volatile int&&>);
static_assert(std::is_same_v<ForwardLikeResult<int, const volatile int&>, const volatile int&&>);
static_assert(std::is_same_v<ForwardLikeResult<int, int&&>, int&&>);
static_assert(std::is_same_v<ForwardLikeResult<int, const int&&>, const int&&>);
static_assert(std::is_same_v<ForwardLikeResult<int, volatile int&&>, volatile int&&>);
static_assert(std::is_same_v<ForwardLikeResult<int, const volatile int&&>, const volatile int&&>);

static_assert(std::is_same_v<ForwardLikeResult<const int, int>, const int&&>);
static_assert(std::is_same_v<ForwardLikeResult<const int, const int>, const int&&>);
static_assert(std::is_same_v<ForwardLikeResult<const int, volatile int>, const volatile int&&>);
static_assert(std::is_same_v<ForwardLikeResult<const int, const volatile int>, const volatile int&&>);
static_assert(std::is_same_v<ForwardLikeResult<const int, int&>, const int&&>);
static_assert(std::is_same_v<ForwardLikeResult<const int, const int&>, const int&&>);
static_assert(std::is_same_v<ForwardLikeResult<const int, volatile int&>, const volatile int&&>);
static_assert(std::is_same_v<ForwardLikeResult<const int, const volatile int&>, const volatile int&&>);
static_assert(std::is_same_v<ForwardLikeResult<const int, int&&>, const int&&>);
static_assert(std::is_same_v<ForwardLikeResult<const int, const int&&>, const int&&>);
static_assert(std::is_same_v<ForwardLikeResult<const int, volatile int&&>, const volatile int&&>);
static_assert(std::is_same_v<ForwardLikeResult<const int, const volatile int&&>, const volatile int&&>);

static_assert(std::is_same_v<ForwardLikeResult<volatile int, int>, int&&>);
static_assert(std::is_same_v<ForwardLikeResult<volatile int, const int>, const int&&>);
static_assert(std::is_same_v<ForwardLikeResult<volatile int, volatile int>, volatile int&&>);
static_assert(std::is_same_v<ForwardLikeResult<volatile int, const volatile int>, const volatile int&&>);
static_assert(std::is_same_v<ForwardLikeResult<volatile int, int&>, int&&>);
static_assert(std::is_same_v<ForwardLikeResult<volatile int, const int&>, const int&&>);
static_assert(std::is_same_v<ForwardLikeResult<volatile int, volatile int&>, volatile int&&>);
static_assert(std::is_same_v<ForwardLikeResult<volatile int, const volatile int&>, const volatile int&&>);
static_assert(std::is_same_v<ForwardLikeResult<volatile int, int&&>, int&&>);
static_assert(std::is_same_v<ForwardLikeResult<volatile int, const int&&>, const int&&>);
static_assert(std::is_same_v<ForwardLikeResult<volatile int, volatile int&&>, volatile int&&>);
static_assert(std::is_same_v<ForwardLikeResult<volatile int, const volatile int&&>, const volatile int&&>);

static_assert(std::is_same_v<ForwardLikeResult<const volatile int, int>, const int&&>);
static_assert(std::is_same_v<ForwardLikeResult<const volatile int, const int>, const int&&>);
static_assert(std::is_same_v<ForwardLikeResult<const volatile int, volatile int>, const volatile int&&>);
static_assert(std::is_same_v<ForwardLikeResult<const volatile int, const volatile int>, const volatile int&&>);
static_assert(std::is_same_v<ForwardLikeResult<const volatile int, int&>, const int&&>);
static_assert(std::is_same_v<ForwardLikeResult<const volatile int, const int&>, const int&&>);
static_assert(std::is_same_v<ForwardLikeResult<const volatile int, volatile int&>, const volatile int&&>);
static_assert(std::is_same_v<ForwardLikeResult<const volatile int, const volatile int&>, const volatile int&&>);
static_assert(std::is_same_v<ForwardLikeResult<const volatile int, int&&>, const int&&>);
static_assert(std::is_same_v<ForwardLikeResult<const volatile int, const int&&>, const int&&>);
static_assert(std::is_same_v<ForwardLikeResult<const volatile int, volatile int&&>, const volatile int&&>);
static_assert(std::is_same_v<ForwardLikeResult<const volatile int, const volatile int&&>, const volatile int&&>);

static_assert(std::is_same_v<ForwardLikeResult<int&, int>, int&>);
static_assert(std::is_same_v<ForwardLikeResult<int&, const int>, const int&>);
static_assert(std::is_same_v<ForwardLikeResult<int&, volatile int>, volatile int&>);
static_assert(std::is_same_v<ForwardLikeResult<int&, const volatile int>, const volatile int&>);
static_assert(std::is_same_v<ForwardLikeResult<int&, int&>, int&>);
static_assert(std::is_same_v<ForwardLikeResult<int&, const int&>, const int&>);
static_assert(std::is_same_v<ForwardLikeResult<int&, volatile int&>, volatile int&>);
static_assert(std::is_same_v<ForwardLikeResult<int&, const volatile int&>, const volatile int&>);
static_assert(std::is_same_v<ForwardLikeResult<int&, int&&>, int&>);
static_assert(std::is_same_v<ForwardLikeResult<int&, const int&&>, const int&>);
static_assert(std::is_same_v<ForwardLikeResult<int&, volatile int&&>, volatile int&>);
static_assert(std::is_same_v<ForwardLikeResult<int&, const volatile int&&>, const volatile int&>);

static_assert(std::is_same_v<ForwardLikeResult<const int&, int>, const int&>);
static_assert(std::is_same_v<ForwardLikeResult<const int&, const int>, const int&>);
static_assert(std::is_same_v<ForwardLikeResult<const int&, volatile int>, const volatile int&>);
static_assert(std::is_same_v<ForwardLikeResult<const int&, const volatile int>, const volatile int&>);
static_assert(std::is_same_v<ForwardLikeResult<const int&, int&>, const int&>);
static_assert(std::is_same_v<ForwardLikeResult<const int&, const int&>, const int&>);
static_assert(std::is_same_v<ForwardLikeResult<const int&, volatile int&>, const volatile int&>);
static_assert(std::is_same_v<ForwardLikeResult<const int&, const volatile int&>, const volatile int&>);
static_assert(std::is_same_v<ForwardLikeResult<const int&, int&&>, const int&>);
static_assert(std::is_same_v<ForwardLikeResult<const int&, const int&&>, const int&>);
static_assert(std::is_same_v<ForwardLikeResult<const int&, volatile int&&>, const volatile int&>);
static_assert(std::is_same_v<ForwardLikeResult<const int&, const volatile int&&>, const volatile int&>);

static_assert(std::is_same_v<ForwardLikeResult<volatile int&, int>, int&>);
static_assert(std::is_same_v<ForwardLikeResult<volatile int&, const int>, const int&>);
static_assert(std::is_same_v<ForwardLikeResult<volatile int&, volatile int>, volatile int&>);
static_assert(std::is_same_v<ForwardLikeResult<volatile int&, const volatile int>, const volatile int&>);
static_assert(std::is_same_v<ForwardLikeResult<volatile int&, int&>, int&>);
static_assert(std::is_same_v<ForwardLikeResult<volatile int&, const int&>, const int&>);
static_assert(std::is_same_v<ForwardLikeResult<volatile int&, volatile int&>, volatile int&>);
static_assert(std::is_same_v<ForwardLikeResult<volatile int&, const volatile int&>, const volatile int&>);
static_assert(std::is_same_v<ForwardLikeResult<volatile int&, int&&>, int&>);
static_assert(std::is_same_v<ForwardLikeResult<volatile int&, const int&&>, const int&>);
static_assert(std::is_same_v<ForwardLikeResult<volatile int&, volatile int&&>, volatile int&>);
static_assert(std::is_same_v<ForwardLikeResult<volatile int&, const volatile int&&>, const volatile int&>);

static_assert(std::is_same_v<ForwardLikeResult<const volatile int&, int>, const int&>);
static_assert(std::is_same_v<ForwardLikeResult<const volatile int&, const int>, const int&>);
static_assert(std::is_same_v<ForwardLikeResult<const volatile int&, volatile int>, const volatile int&>);
static_assert(std::is_same_v<ForwardLikeResult<const volatile int&, const volatile int>, const volatile int&>);
static_assert(std::is_same_v<ForwardLikeResult<const volatile int&, int&>, const int&>);
static_assert(std::is_same_v<ForwardLikeResult<const volatile int&, const int&>, const int&>);
static_assert(std::is_same_v<ForwardLikeResult<const volatile int&, volatile int&>, const volatile int&>);
static_assert(std::is_same_v<ForwardLikeResult<const volatile int&, const volatile int&>, const volatile int&>);
static_assert(std::is_same_v<ForwardLikeResult<const volatile int&, int&&>, const int&>);
static_assert(std::is_same_v<ForwardLikeResult<const volatile int&, const int&&>, const int&>);
static_assert(std::is_same_v<ForwardLikeResult<const volatile int&, volatile int&&>, const volatile int&>);
static_assert(std::is_same_v<ForwardLikeResult<const volatile int&, const volatile int&&>, const volatile int&>);

static_assert(std::is_same_v<ForwardLikeResult<int&&, int>, int&&>);
static_assert(std::is_same_v<ForwardLikeResult<int&&, const int>, const int&&>);
static_assert(std::is_same_v<ForwardLikeResult<int&&, volatile int>, volatile int&&>);
static_assert(std::is_same_v<ForwardLikeResult<int&&, const volatile int>, const volatile int&&>);
static_assert(std::is_same_v<ForwardLikeResult<int&&, int&>, int&&>);
static_assert(std::is_same_v<ForwardLikeResult<int&&, const int&>, const int&&>);
static_assert(std::is_same_v<ForwardLikeResult<int&&, volatile int&>, volatile int&&>);
static_assert(std::is_same_v<ForwardLikeResult<int&&, const volatile int&>, const volatile int&&>);
static_assert(std::is_same_v<ForwardLikeResult<int&&, int&&>, int&&>);
static_assert(std::is_same_v<ForwardLikeResult<int&&, const int&&>, const int&&>);
static_assert(std::is_same_v<ForwardLikeResult<int&&, volatile int&&>, volatile int&&>);
static_assert(std::is_same_v<ForwardLikeResult<int&&, const volatile int&&>, const volatile int&&>);

static_assert(std::is_same_v<ForwardLikeResult<const int&&, int>, const int&&>);
static_assert(std::is_same_v<ForwardLikeResult<const int&&, const int>, const int&&>);
static_assert(std::is_same_v<ForwardLikeResult<const int&&, volatile int>, const volatile int&&>);
static_assert(std::is_same_v<ForwardLikeResult<const int&&, const volatile int>, const volatile int&&>);
static_assert(std::is_same_v<ForwardLikeResult<const int&&, int&>, const int&&>);
static_assert(std::is_same_v<ForwardLikeResult<const int&&, const int&>, const int&&>);
static_assert(std::is_same_v<ForwardLikeResult<const int&&, volatile int&>, const volatile int&&>);
static_assert(std::is_same_v<ForwardLikeResult<const int&&, const volatile int&>, const volatile int&&>);
static_assert(std::is_same_v<ForwardLikeResult<const int&&, int&&>, const int&&>);
static_assert(std::is_same_v<ForwardLikeResult<const int&&, const int&&>, const int&&>);
static_assert(std::is_same_v<ForwardLikeResult<const int&&, volatile int&&>, const volatile int&&>);
static_assert(std::is_same_v<ForwardLikeResult<const int&&, const volatile int&&>, const volatile int&&>);

static_assert(std::is_same_v<ForwardLikeResult<volatile int&&, int>, int&&>);
static_assert(std::is_same_v<ForwardLikeResult<volatile int&&, const int>, const int&&>);
static_assert(std::is_same_v<ForwardLikeResult<volatile int&&, volatile int>, volatile int&&>);
static_assert(std::is_same_v<ForwardLikeResult<volatile int&&, const volatile int>, const volatile int&&>);
static_assert(std::is_same_v<ForwardLikeResult<volatile int&&, int&>, int&&>);
static_assert(std::is_same_v<ForwardLikeResult<volatile int&&, const int&>, const int&&>);
static_assert(std::is_same_v<ForwardLikeResult<volatile int&&, volatile int&>, volatile int&&>);
static_assert(std::is_same_v<ForwardLikeResult<volatile int&&, const volatile int&>, const volatile int&&>);
static_assert(std::is_same_v<ForwardLikeResult<volatile int&&, int&&>, int&&>);
static_assert(std::is_same_v<ForwardLikeResult<volatile int&&, const int&&>, const int&&>);
static_assert(std::is_same_v<ForwardLikeResult<volatile int&&, volatile int&&>, volatile int&&>);
static_assert(std::is_same_v<ForwardLikeResult<volatile int&&, const volatile int&&>, const volatile int&&>);

static_assert(std::is_same_v<ForwardLikeResult<const volatile int&&, int>, const int&&>);
static_assert(std::is_same_v<ForwardLikeResult<const volatile int&&, const int>, const int&&>);
static_assert(std::is_same_v<ForwardLikeResult<const volatile int&&, volatile int>, const volatile int&&>);
static_assert(std::is_same_v<ForwardLikeResult<const volatile int&&, const volatile int>, const volatile int&&>);
static_assert(std::is_same_v<ForwardLikeResult<const volatile int&&, int&>, const int&&>);
static_assert(std::is_same_v<ForwardLikeResult<const volatile int&&, const int&>, const int&&>);
static_assert(std::is_same_v<ForwardLikeResult<const volatile int&&, volatile int&>, const volatile int&&>);
static_assert(std::is_same_v<ForwardLikeResult<const volatile int&&, const volatile int&>, const volatile int&&>);
static_assert(std::is_same_v<ForwardLikeResult<const volatile int&&, int&&>, const int&&>);
static_assert(std::is_same_v<ForwardLikeResult<const volatile int&&, const int&&>, const int&&>);
static_assert(std::is_same_v<ForwardLikeResult<const volatile int&&, volatile int&&>, const volatile int&&>);
static_assert(std::is_same_v<ForwardLikeResult<const volatile int&&, const volatile int&&>, const volatile int&&>);

}  // namespace

USERVER_NAMESPACE_END
