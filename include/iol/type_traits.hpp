#ifndef IOL_TYPE_TRAITS_HPP
#define IOL_TYPE_TRAITS_HPP

#include <type_traits>

namespace iol
{

template <typename From, typename To>
struct copy_const
{
  using type = std::conditional_t<std::is_const_v<From>, std::add_const_t<To>, To>;
};

template <typename From, typename To>
using copy_const_t = typename copy_const<From, To>::type;

template <typename From, typename To>
struct copy_volatile
{
  using type = std::conditional_t<std::is_volatile_v<From>, std::add_volatile_t<To>, To>;
};

template <typename From, typename To>
using copy_volatile_t = typename copy_volatile<From, To>::type;

template <typename From, typename To>
struct copy_cv
{
  using type = copy_const_t<From, copy_volatile_t<From, To>>;
};

template <typename From, typename To>
using copy_cv_t = typename copy_cv<From, To>::type;

template <typename From, typename To>
struct copy_reference
{
  using type = std::conditional_t<
      std::is_rvalue_reference_v<From>, std::add_rvalue_reference_t<To>,
      std::conditional_t<std::is_lvalue_reference_v<From>, std::add_lvalue_reference_t<To>, To>>;
};

template <typename From, typename To>
using copy_reference_t = typename copy_reference<From, To>::type;

template <typename From, typename To>
struct copy_cvref
{
  using type = copy_reference_t<From, copy_cv_t<std::remove_reference_t<From>, To>>;
};

template <typename From, typename To>
using copy_cvref_t = typename copy_cvref<From, To>::type;

namespace _callable
{

template <typename Function, typename... Args>
concept callable = requires(Function&& function, Args&&... args)
{
  ((Function &&) function)((Args &&) args...);
};

template <typename Function, typename... Args>
concept nothrow_callable = callable<Function, Args...> &&
    requires(Function&& function, Args&&... args)
{
  {
    ((Function &&) function)((Args &&) args...)
  }
  noexcept;
};

}  // namespace _callable

template <typename Function, typename... Args>
using is_callable = std::bool_constant<_callable::callable<Function, Args...>>;

template <typename Function, typename... Args>
inline constexpr auto is_callable_v = is_callable<Function, Args...>::value;

template <typename Function, typename... Args>
using is_nothrow_callable = std::bool_constant<_callable::nothrow_callable<Function, Args...>>;

template <typename Function, typename... Args>
inline constexpr auto is_nothrow_callable_v = is_nothrow_callable<Function, Args...>::value;

template <typename Function, typename... Args>
using call_result_t = decltype(std::declval<Function>()(std::declval<Args>()...));

}  // namespace iol

#endif  // IOL_TYPE_TRAITS_HPP
