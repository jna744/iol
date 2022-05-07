#ifndef IOL_EXECUTION_SCHEDULER_HPP
#define IOL_EXECUTION_SCHEDULER_HPP

#include <iol/tag_invoke.hpp>

#include <iol/execution/sender.hpp>
#include <iol/execution/receiver.hpp>

#include <concepts>
#include <type_traits>
#include <utility>

namespace iol::execution
{

namespace _scheduler
{

template <typename CPO>
struct get_completion_scheduler_t
{
  template <typename S>
    requires(
        sender<S> &&
        (std::same_as<set_value_t, CPO> || std::same_as<set_error_t, CPO> ||
         std::same_as<
             set_stopped_t,
             CPO>)&&tag_invocable<get_completion_scheduler_t<CPO>, std::remove_cvref_t<S> const&>)
  constexpr auto operator()(S&& s) const noexcept
  {
    static_assert(
        nothrow_tag_invocable<get_completion_scheduler_t<CPO>, std::remove_cvref_t<S> const&>);
    return tag_invoke(*this, std::as_const(s));
  }
};

struct schedule_t
{

  template <typename S>
    requires tag_invocable<schedule_t, S> && sender<tag_invoke_result_t<schedule_t, S>> && requires(
        get_completion_scheduler_t<set_value_t> const tag,
        tag_invoke_result_t<schedule_t, S>&&          sender)
    {
      {
        tag_invoke(tag, sender)
        } -> std::same_as<std::remove_cvref_t<S>>;
    }
  constexpr sender auto operator()(S&& s) const noexcept(nothrow_tag_invocable<schedule_t, S>)
  {
    return tag_invoke(*this, (S &&) s);
  }
};

struct forwarding_scheduler_query_t
{
  template <typename T>
  constexpr bool operator()(T&& t) const noexcept
  {
    if constexpr (tag_invocable<forwarding_scheduler_query_t, T>) {
      if constexpr (std::convertible_to<
                        tag_invoke_result_t<forwarding_scheduler_query_t, T>, bool>) {
        static_assert(nothrow_tag_invocable<forwarding_scheduler_query_t, T>);
        return tag_invoke(*this, (T &&) t);
      } else {
        return false;
      }
    } else {
      return false;
    }
  }
};

}  // namespace _scheduler

using _scheduler::get_completion_scheduler_t;

using _scheduler::schedule_t;

using _scheduler::forwarding_scheduler_query_t;

template <typename CPO>
inline constexpr get_completion_scheduler_t<CPO> get_completion_scheduler;

inline constexpr schedule_t schedule{};

inline constexpr forwarding_scheduler_query_t forwarding_scheduler_query{};

template <typename S>
concept scheduler = std::copy_constructible<std::remove_cvref_t<S>> &&
    std::equality_comparable<std::remove_cvref_t<S>> &&
    requires(S&& s, get_completion_scheduler_t<set_value_t> const tag)
{
  {
    schedule((S &&) s)
    } -> sender;
  {
    tag_invoke(tag, schedule((S &&) s))
    } -> std::same_as<std::remove_cvref_t<S>>;
};

template <scheduler S>
using schedule_result_t = decltype(schedule(std::declval<S>()));

enum class forward_progress_guarantee { concurrent, parallel, weakly_parallel };

namespace _scheduler
{

struct get_forward_progress_guarantee_t
{

  template <sender S>
  constexpr forward_progress_guarantee operator()(S&& s) const noexcept
  {
    if constexpr (tag_invocable<get_forward_progress_guarantee_t, std::add_const_t<S>>) {
      if constexpr (std::same_as<
                        tag_invoke_result_t<get_forward_progress_guarantee_t, std::add_const_t<S>>,
                        forward_progress_guarantee>) {
        static_assert(nothrow_tag_invocable<get_forward_progress_guarantee_t, std::add_const_t<S>>);
        return tag_invoke(*this, std::as_const(s));
      } else {
        return forward_progress_guarantee::weakly_parallel;
      }
    } else {
      return forward_progress_guarantee::weakly_parallel;
    }
  }
};

}  // namespace _scheduler

using _scheduler::get_forward_progress_guarantee_t;

inline constexpr get_forward_progress_guarantee_t get_forward_progress_guarantee{};

}  // namespace iol::execution

#endif  // IOL_EXECUTION_SCHEDULER_HPP
