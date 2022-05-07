#ifndef IOL_EXECUTION_READ_HPP
#define IOL_EXECUTION_READ_HPP

#include <iol/type_traits.hpp>

#include <iol/execution/completion_signatures.hpp>
#include <iol/execution/env.hpp>
#include <iol/execution/receiver.hpp>
#include <iol/execution/sender.hpp>

#include <exception>
#include <type_traits>
#include <utility>

namespace iol::execution
{

namespace _read
{

template <typename Tag>
struct read_sender
{

  template <typename R>
  struct operation_state
  {
    R receiver_;

    friend void tag_invoke(start_t, operation_state& state) noexcept
    try {
      decltype(auto) env = get_env(state.receiver_);
      set_value(std::move(state.receiver_), Tag{}(env));
    } catch (...) {
      set_error(std::move(state.receiver_), std::current_exception());
    }
  };

  template <receiver R>
  friend operation_state<std::decay_t<R>> tag_invoke(connect_t, read_sender, R&& receiver)
  {
    return {(R &&) receiver};
  }

  friend dependent_completion_signatures<no_env> tag_invoke(
      get_completion_signatures_t, read_sender, auto);

  template <typename Env>
    requires(!std::same_as<Env, no_env> && is_callable_v<Tag, Env>)
  friend auto tag_invoke(get_completion_signatures_t, read_sender, Env) -> completion_signatures<
      set_value_t(call_result_t<Tag, Env>), set_error_t(std::exception_ptr)>;
};

struct read_t
{
  template <typename Tag>
  constexpr auto operator()(Tag) const noexcept
  {
    return read_sender<Tag>{};
  }
};

}  // namespace _read

using _read::read_t;

inline constexpr read_t read{};

}  // namespace iol::execution

#endif  // IOL_EXECUTION_READ_HPP
