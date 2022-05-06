#ifndef IOL_EXECUTION_ENV_HPP
#define IOL_EXECUTION_ENV_HPP

#include <concepts>
#include <iol/tag_invoke.hpp>
#include <type_traits>

namespace iol::execution
{

namespace env_impl
{

struct no_env
{
  friend void tag_invoke(auto, std::same_as<no_env> auto, auto&&...) = delete;
};

struct empty_env
{};

struct get_env_t
{

  template <typename Receiver>
    requires tag_invocable<get_env_t, Receiver>
  constexpr auto operator()(Receiver&& receiver) const -> tag_invoke_result_t<get_env_t, Receiver>
  {
    return tag_invoke(get_env_t{}, (Receiver &&) receiver);
  }

  template <typename Receiver>
    requires(!tag_invocable<get_env_t, Receiver>)
  constexpr auto operator()(Receiver&& receiver) const { return empty_env{}; }
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

}  // namespace env_impl

using env_impl::empty_env;

using env_impl::no_env;

using env_impl::forwarding_env_query_t;

using env_impl::get_env_t;

inline constexpr get_env_t get_env{};

inline constexpr forwarding_env_query_t forwarding_env_query{};

template <typename R>
using env_of_t = decltype(get_env(std::declval<R>()));

}  // namespace iol::execution

#endif  // IOL_EXECUTION_ENV_HPP