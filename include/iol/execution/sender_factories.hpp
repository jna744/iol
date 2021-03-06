#ifndef IOL_EXECUTION_SENDER_FACTORIES_HPP
#define IOL_EXECUTION_SENDER_FACTORIES_HPP

#include <iol/execution/completion_signatures.hpp>
#include <iol/execution/receiver.hpp>

#include <tuple>
#include <utility>
#include <exception>
#include <type_traits>

namespace iol::execution
{

namespace _just
{

template <typename... Ts>
struct just_sender
{

  using completion_signatures =
      execution::completion_signatures<set_value_t(Ts...), set_error_t(std::exception_ptr)>;

  using value_tuple = std::tuple<Ts...>;
  [[no_unique_address]] value_tuple values_;

  template <typename Receiver>
  struct operation_state
  {

    [[no_unique_address]] value_tuple values_;
    [[no_unique_address]] Receiver    receiver_;

    friend void tag_invoke(start_t, operation_state& os) noexcept
    {
      try {
        std::apply(
            [&](Ts&... ts) { set_value(std::move(os.receiver_), std::move(ts)...); }, os.values_);
      } catch (...) {
        set_error(std::move(os.receiver_), std::current_exception());
      }
    }
  };

  template <receiver R>
    requires(receiver_of<R, Ts...> && (std::copy_constructible<Ts> && ...))
  friend operation_state<std::remove_cvref_t<R>> tag_invoke(
      connect_t, just_sender const& sender,
      R&& receiver)
    // noexcept(std::is_nothrow_constructible_v<value_tuple, value_tuple const&>&&
    //                              std::is_nothrow_move_constructible_v<std::remove_cvref_t<R>>)
  {
    return {sender.values_, (R &&) receiver};
  }

  template <receiver R>
    requires(receiver_of<R, Ts...> && (std::copy_constructible<Ts> && ...))
  friend operation_state<std::remove_cvref_t<R>> tag_invoke(
      connect_t, just_sender&& sender, R&& receiver)
    // noexcept(std::is_nothrow_constructible_v<value_tuple, value_tuple&&>&&
    //                              std::is_nothrow_move_constructible_v<std::remove_cvref_t<R>>)
  {
    return {std::move(sender.values_), (R &&) receiver};
  }
};

}  // namespace _just

template <typename... Ts>
  requires(
      (std::move_constructible<std::decay_t<Ts>> &&
       std::constructible_from<std::decay_t<Ts>, Ts>)&&...)
_just::just_sender<std::decay_t<Ts>...> just(Ts&&... ts)
noexcept((std::is_nothrow_constructible_v<std::decay_t<Ts>, Ts> && ...))
{
  return {{(Ts &&) ts...}};
}

}  // namespace iol::execution

#endif  // IOL_EXECUTION_SENDER_FACTORIES_HPP
