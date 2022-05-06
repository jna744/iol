#ifndef IOL_EXECUTION_GENERAL_QUERIES_HPP
#define IOL_EXECUTION_GENERAL_QUERIES_HPP

#include <iol/execution/env.hpp>
#include <iol/execution/read.hpp>
#include <iol/execution/scheduler.hpp>
#include <iol/execution/sender.hpp>

#include <iol/tag_invoke.hpp>

#include <concepts>
#include <type_traits>
#include <utility>

namespace iol::execution
{

namespace general_queries_impl
{

template <typename Env>
concept no_environment = std::same_as<std::remove_cvref<Env>, no_env>;

template <typename Tag, typename Env>
concept valid_scheduler_ret = scheduler<tag_invoke_result_t<Tag, std::add_const_t<Env>>>;

struct get_scheduler_t
{

  template <no_environment Env>
  constexpr void operator()(Env&& t) const = delete;

  template <typename Env>
    requires tag_invocable<get_scheduler_t, std::add_const_t<Env>> &&
        valid_scheduler_ret<get_scheduler_t, Env>
  constexpr scheduler auto operator()(Env&& r) const noexcept
  {
    static_assert(nothrow_tag_invocable<get_scheduler_t, std::add_const_t<Env>>);
    return tag_invoke(*this, std::as_const(r));
  }

  constexpr sender auto operator()() const noexcept { return read(*this); }
};

struct get_delegate_scheduler_t
{

  template <no_environment Env>
  constexpr void operator()(Env&& t) const = delete;

  template <typename Env>
    requires tag_invocable<get_delegate_scheduler_t, std::add_const_t<Env>> &&
        valid_scheduler_ret<get_delegate_scheduler_t, Env>
  constexpr scheduler auto operator()(Env&& r) const noexcept
  {
    static_assert(nothrow_tag_invocable<get_delegate_scheduler_t, std::add_const_t<Env>>);
    return tag_invoke(*this, std::as_const(r));
  }

  constexpr sender auto operator()() const noexcept { return read(*this); }
};

struct get_allocator_t
{

  template <no_environment Env>
  constexpr void operator()(Env&& t) const = delete;

  // TODO:
  // assert the returned value is an allocator
  template <typename Env>
    requires tag_invocable<get_allocator_t, std::add_const_t<Env>>
  constexpr auto operator()(Env&& r) const noexcept
  {
    static_assert(nothrow_tag_invocable<get_allocator_t, std::add_const_t<Env>>);
    return tag_invoke(*this, std::as_const(r));
  }

  constexpr sender auto operator()() const noexcept { return read(*this); }
};

// TODO:
// struct get_stop_token_t
// {
// };

}  // namespace general_queries_impl

using general_queries_impl::get_allocator_t;
using general_queries_impl::get_delegate_scheduler_t;
using general_queries_impl::get_scheduler_t;
// TODO:
// using general_queries__::get_stop_token_t;

inline constexpr get_scheduler_t          get_scheduler{};
inline constexpr get_delegate_scheduler_t get_delegate_scheduler{};
inline constexpr get_allocator_t          get_allocator{};
// TODO:
// inline constexpr stop_token_t get_stop_token{}

}  // namespace iol::execution

#endif  // IOL_EXECUTION_GENERAL_QUERIES_HPP
