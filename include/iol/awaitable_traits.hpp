#ifndef IOL_AWAITABLE_TRAITS_HPP
#define IOL_AWAITABLE_TRAITS_HPP

#include <iol/is_awaitable.hpp>

//

#include <type_traits>

namespace iol
{

namespace detail
{

template <typename T>
concept with_co_await_member = requires(T&& t)
{
  {
    ((T &&) t).operator co_await()
    } -> is_awaitable;
};

template <typename T>
concept with_co_await_free = requires(T&& t)
{
  {
    operator co_await((T &&) t)
    } -> is_awaitable;
};

template <typename T>
struct infer_awaitable_type {
};

template <typename T>
requires with_co_await_member<T>
struct infer_awaitable_type<T> {
  using type = decltype(std::declval<T>().operator co_await());
};

template <typename T>
requires(with_co_await_free<T> && !with_co_await_member<T>) struct infer_awaitable_type<T> {
  using type = decltype(operator co_await(std::declval<T>()));
};

template <typename T>
requires(
    !with_co_await_free<T> && !with_co_await_member<T> &&
    is_awaitable<T>) struct infer_awaitable_type<T> {
  using type = T;
};

template <typename T>
using infer_awaitable_t = typename infer_awaitable_type<T>::type;

}  // namespace detail

template <typename T>
struct awaitable_traits {
};

template <typename T>
requires requires
{
  typename detail::infer_awaitable_t<T>;
}
struct awaitable_traits<T> {

  using awaiter_type = detail::infer_awaitable_t<T>;

  using result_type = decltype(std::declval<awaiter_type>().await_resume());
};

template <typename T>
using awaitable_result_t = typename awaitable_traits<T>::result_type;

}  // namespace iol

#endif  // IOL_AWAITABLE_TRAITS_HPP
