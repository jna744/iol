#ifndef IOL_EXECUTION_THEN_HPP
#define IOL_EXECUTION_THEN_HPP

#include <iol/execution/sender.hpp>
#include <iol/execution/receiver.hpp>
#include <iol/execution/receiver_adaptor.hpp>
#include <iol/execution/sender_adaptor_closure.hpp>
#include <iol/execution/connect.hpp>
#include <iol/execution/completion_signatures.hpp>
#include <iol/execution/scheduler.hpp>

#include <iol/concepts.hpp>

#include <functional>
#include <exception>
#include <concepts>

namespace iol::execution
{

namespace then_impl
{

template <typename R, typename F>
class then_receiver : public receiver_adaptor<then_receiver<R, F>, R>
{
 public:

  // friend receiver_adaptor<then_receiver, F>;

  [[no_unique_address]] F function_;

  template <typename... Args>
    requires receiver_of<R, std::invoke_result_t<F, Args...>>
  void set_value(Args&&... args) && noexcept
  try {
    execution::set_value(
        std::move(*this).base(), std::invoke(std::move(function_), (Args &&) args...));
  } catch (...) {
    execution::set_error(std::move(*this).base(), std::current_exception());
  }

  template <typename... Args>
    requires(
        receiver_of<R>&& std::same_as<std::remove_cvref_t<std::invoke_result<F, Args...>>, void>)
  void set_value(Args&&... args) && noexcept
  try {
    std::invoke(std::move(function_));
    execution::set_value(std::move(*this).base());
  } catch (...) {
    execution::set_error(std::move(*this).base(), std::current_exception());
  }

 public:

  constexpr then_receiver(R r, F f)
    : receiver_adaptor<then_receiver<R, F>, R>{std::move(r)}, function_{std::move(f)}
  {}
};

template <sender S, typename F>
struct then_sender
{
  S sender_;
  F function_;

  template <receiver R>
  // requires sender_to<S, then_receiver<R, F>>
  friend constexpr auto tag_invoke(connect_t, then_sender&& self, R r)
      -> connect_result_t<S, then_receiver<R, F>>
  {
    return execution::connect(
        std::move(self.sender_), then_receiver<R, F>{std::move(r), std::move(self.function_)});
  }

  template <typename... Args>
  using set_value_tuple_helper = set_value_t(std::invoke_result_t<F, Args...>);

  template <typename... Args>
  using set_value_tuple = typename std::conditional_t<
      !std::is_same_v<std::invoke_result_t<F, Args...>, void>,
      meta::m_defer<set_value_tuple_helper, Args...>, std::type_identity<set_value_t()>>::type;

  template <typename Env>
  friend auto tag_invoke(get_completion_signatures_t, then_sender&&, Env)
      -> make_completion_signatures<
          S, Env, completion_signatures<set_error_t(std::exception_ptr)>, set_value_tuple>;
};

template <typename CPO, typename S>
using completion_scheduler_result_t = decltype(get_completion_scheduler<CPO>(std::declval<S>()));

template <typename CPO, typename S, typename F>
concept completion_scheduler_tag_invocable = requires
{
  typename completion_scheduler_result_t<set_value_t, S>;
}
&&tag_invocable<CPO, completion_scheduler_result_t<set_value_t, S>, S, F>;

struct then_t
{

  template <sender S, typename Function>
    requires movable_type<std::decay_t<Function>> &&
        then_impl::completion_scheduler_tag_invocable<then_t, S, Function>
  constexpr sender auto operator()(S&& s, Function&& function) const
  {
    return tag_invoke(
        *this, get_completion_scheduler<set_value_t>((S &&) s), (S &&) s, (Function &&) function);
  }

  template <sender S, typename Function>
    requires(
        movable_type<std::decay_t<Function>> &&
        (!then_impl::completion_scheduler_tag_invocable<then_t, S, Function>)&&tag_invocable<
            then_t, S, Function>)
  constexpr sender auto operator()(S&& s, Function&& function) const
  {
    return tag_invoke(*this, (S &&) s, (Function &&) function);
  }

  template <sender S, typename Function>
    requires(
        movable_type<std::decay_t<Function>> &&
        !(then_impl::completion_scheduler_tag_invocable<then_t, S, Function> ||
          tag_invocable<then_t, S, Function>))
  constexpr sender auto operator()(S&& s, Function&& function) const
  {
    return then_impl::then_sender<std::decay_t<S>, std::decay_t<Function>>{
        (S &&) s, (Function &&) function};
  }

  template <typename Function>
  constexpr sender_adaptor_closure<then_t, std::decay_t<Function>> operator()(
      Function&& function) const
  {
    return {{}, *this, (Function &&) function};
  }
};

}  // namespace then_impl

using then_impl::then_t;

inline constexpr then_t then{};

}  // namespace iol::execution

#endif  // IOL_EXECUTION_THEN_HPP
