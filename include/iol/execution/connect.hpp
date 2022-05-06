#ifndef IOL_EXECUTION_CONNECT_HPP
#define IOL_EXECUTION_CONNECT_HPP

#include <iol/execution/receiver.hpp>
#include <iol/execution/sender.hpp>
#include <iol/execution/start.hpp>  // operation_state
#include <iol/tag_invoke.hpp>

#include <concepts>
#include <type_traits>

namespace iol::execution
{

namespace connect_impl
{

struct connect_t
{

  template <sender S, receiver R>
    requires tag_invocable<connect_t, S, R> && operation_state<tag_invoke_result_t<connect_t, S, R>>
  constexpr auto operator()(S&& sender, R&& receiver) const
      noexcept(nothrow_tag_invocable<connect_t, S, R>) -> operation_state auto
  {
    return tag_invoke(connect_t{}, (S &&) sender, (R &&) receiver);
  }
};

}  // namespace connect_impl

using connect_impl::connect_t;

inline constexpr connect_t connect{};

template <typename S, typename R>
using connect_result_t = decltype(connect(std::declval<S>(), std::declval<R>()));

}  // namespace iol::execution

#endif  // IOL_EXECUTION_CONNECT_HPP
