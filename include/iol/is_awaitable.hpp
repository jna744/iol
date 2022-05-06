#ifndef IOL_IS_AWAITABLE_HPP
#define IOL_IS_AWAITABLE_HPP

#include <concepts>
#include <coroutine>
#include <type_traits>

namespace iol
{

template <typename T>
struct is_coroutine_handle : std::false_type
{};

template <typename T>
struct is_coroutine_handle<std::coroutine_handle<T>> : std::true_type
{};

template <>
struct is_coroutine_handle<std::coroutine_handle<>> : std::true_type
{};

template <typename T>
  requires(!std::is_same_v<T, std::remove_cvref_t<T>>)
struct is_coroutine_handle<T> : is_coroutine_handle<std::remove_cvref_t<T>>
{};

template <typename T>
inline constexpr bool is_coroutine_handle_v = is_coroutine_handle<T>::value;

namespace detail
{

struct invalid_await_suspend_arg
{};

template <typename T>
struct await_suspend_arg_type
{

 private:

  struct invalid_await_suspend
  {};

  template <typename Ret, typename U, typename Arg>
  static constexpr auto infer(Ret (U::*)(Arg)) -> std::type_identity<Arg>;

  static constexpr auto infer(...) -> invalid_await_suspend;

  template <typename U, typename = void>
  struct helper
  {
    using type = invalid_await_suspend_arg;
  };

  template <typename U>
  struct helper<
      U, std::void_t<typename decltype(infer(&std::remove_cvref_t<U>::await_suspend))::type>>
  {
    using type = typename decltype(infer(&std::remove_cvref_t<U>::await_suspend))::type;
  };

 public:

  using type = typename helper<T>::type;
};

template <typename T>
using await_suspend_arg_type_t = typename await_suspend_arg_type<T>::type;

template <typename T>
concept valid_await_suspend_ret =
    std::same_as<void, T> || std::same_as<bool, T> || is_coroutine_handle_v<T>;  //

}  // namespace detail

template <typename T>
concept is_awaitable = requires
{
  typename detail::await_suspend_arg_type<T>::type;
  requires(!std::same_as<
           detail::invalid_await_suspend_arg, typename detail::await_suspend_arg_type<T>::type>);
}
&&requires(T&& t)
{
  requires is_coroutine_handle_v<detail::await_suspend_arg_type_t<T>>;  //
  {
    t.await_ready()
    } -> std::same_as<bool>;
  {
    t.await_suspend(std::declval<detail::await_suspend_arg_type_t<T>>())
    } -> detail::valid_await_suspend_ret;
  t.await_resume();
};

}  // namespace iol

#endif  // IOL_IS_AWAITABLE_HPP
