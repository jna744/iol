#ifndef IOL_EXECUTION_SENDER_HPP
#define IOL_EXECUTION_SENDER_HPP

#include <iol/meta.hpp>
#include <iol/tag_invoke.hpp>

#include <iol/execution/env.hpp>
#include <iol/execution/receiver.hpp>

#include <concepts>
#include <type_traits>

namespace iol::execution
{

namespace _start
{

struct start_t
{
  template <typename O>
    requires tag_invocable<start_t, O&>
  decltype(auto) operator()(O& o) const noexcept(nothrow_tag_invocable<start_t, O&>)
  {
    static_assert(nothrow_tag_invocable<start_t, O&>);
    return tag_invoke(start_t{}, o);
  }
};

}  // namespace _start

using _start::start_t;

inline constexpr start_t start{};

template <typename O>
concept operation_state = std::destructible<O> && std::is_object_v<O> && requires(O& o)
{
  {
    start(o)
  }
  noexcept;
};

namespace _connect
{

struct connect_t;

}

extern _connect::connect_t const connect;

namespace _completion_signatures
{

template <typename S, typename E>
struct completion_signatures_of_impl;

}

template <typename S, typename E>
using completion_signatures_of_t =
    typename _completion_signatures::completion_signatures_of_impl<S, E>::type;

namespace _sender
{

template <template <template <typename...> class, template <typename...> class> class>
struct has_value_types
{
  using type = void;
};

template <template <template <typename...> class> class>
struct has_error_types
{
  using type = void;
};

template <typename S>
concept has_sender_types = requires
{
  typename has_value_types<S::template value_types>::type;
  typename has_error_types<S::template error_types>::type;
  typename meta::m_bool<S::sends_stopped>;
};

template <typename S, typename E>
concept sender_base = requires
{
  typename completion_signatures_of_t<S, E>;
}
&&has_sender_types<completion_signatures_of_t<S, E>>;

}  // namespace _sender

template <typename S, typename E = no_env>
concept sender = _sender::sender_base<S, E> && _sender::sender_base<S, no_env> &&
    std::move_constructible<std::remove_cvref_t<S>>;

template <typename S, typename R>
concept sender_to = sender<S, env_of_t<R>> && receiver<R> && requires(S&& s, R&& r)
{
  execution::connect((S &&) s, (R &&) r);
};

template <typename S, typename E = no_env, typename... Ts>
concept sender_of =
    (sender<S, E> &&
     std::same_as<
         meta::m_list<Ts...>, typename completion_signatures_of_t<S, E>::template value_types<
                                  meta::m_list, meta::m_identity_t>>);

namespace _connect
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

}  // namespace _connect

using _connect::connect_t;

inline constexpr connect_t connect{};

template <typename S, typename R>
using connect_result_t = decltype(connect(std::declval<S>(), std::declval<R>()));

}  // namespace iol::execution

#endif  // IOL_EXECUTION_SENDER_HPP
