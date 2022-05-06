#ifndef IOL_EXECUTION_SENDER_HPP
#define IOL_EXECUTION_SENDER_HPP

#include <iol/execution/env.hpp>
#include <iol/execution/receiver.hpp>

#include <iol/meta.hpp>

#include <concepts>
#include <type_traits>

namespace iol::execution
{

namespace connect_impl
{

struct connect_t;

}

extern connect_impl::connect_t const connect;

namespace completion_signatures_impl
{

template <typename S, typename E>
struct completion_signatures_of_impl;

}

template <typename S, typename E>
using completion_signatures_of_t =
    typename completion_signatures_impl::completion_signatures_of_impl<S, E>::type;

namespace sender_impl
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

}  // namespace sender_impl

template <typename S, typename E = no_env>
concept sender = sender_impl::sender_base<S, E> && sender_impl::sender_base<S, no_env> &&
    std::move_constructible<std::remove_cvref_t<S>>;

template <typename S, typename R>
concept sender_to = sender<S, env_of_t<R>> && receiver<R> && requires(S &&s, R &&r)
{
  execution::connect((S &&) s, (R &&) r);
};

template <typename S, typename E = no_env, typename... Ts>
concept sender_of =
    (sender<S, E> &&
     std::same_as<
         meta::m_list<Ts...>, typename completion_signatures_of_t<S, E>::template value_types<
                                  meta::m_list, meta::m_identity_t>>);

}  // namespace iol::execution

#endif  // IOL_EXECUTION_SENDER_HPP
