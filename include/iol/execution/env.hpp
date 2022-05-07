#ifndef IOL_EXECUTION_ENV_HPP
#define IOL_EXECUTION_ENV_HPP

#include <iol/tag_invoke.hpp>

#include <type_traits>
#include <concepts>

namespace iol::execution
{

namespace _env
{

struct no_env
{
  friend void tag_invoke(auto, std::same_as<no_env> auto, auto&&...) = delete;
};

struct empty_env
{};

struct get_env_t;

template <typename R>
consteval bool is_nothrow_get_env()
{
  if constexpr (tag_invocable<get_env_t, R>)
    return nothrow_tag_invocable<get_env_t, R>;
  else
    return true;
}

struct get_env_t
{

  template <typename Receiver>
  constexpr decltype(auto) operator()(Receiver&& receiver) const
      noexcept(is_nothrow_get_env<Receiver>())
  {
    if constexpr (tag_invocable<get_env_t, Receiver>) {
      return tag_invoke(*this, (Receiver &&) receiver);
    } else {
      return empty_env{};
    }
  }
};

struct forwarding_env_query_t
{

  template <typename T>
  constexpr bool operator()(T&& t) const noexcept
  {
    if constexpr (tag_invocable<forwarding_env_query_t, T>) {
      static_assert(nothrow_tag_invocable<forwarding_env_query_t, T>);
      if constexpr (std::convertible_to<tag_invoke_result_t<forwarding_env_query_t, T>, bool>) {
        return tag_invoke(forwarding_env_query_t{}, (T &&) t);
      } else {
        return true;
      }
    } else {
      return true;
    }
  }
};

}  // namespace _env

using _env::empty_env;

using _env::no_env;

using _env::forwarding_env_query_t;

using _env::get_env_t;

inline constexpr get_env_t get_env{};

inline constexpr forwarding_env_query_t forwarding_env_query{};

template <typename R>
using env_of_t = decltype(execution::get_env(std::declval<R>()));

}  // namespace iol::execution

#endif  // IOL_EXECUTION_ENV_HPP
