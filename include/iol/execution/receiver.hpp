#ifndef IOL_EXECUTION_RECEIVER_HPP
#define IOL_EXECUTION_RECEIVER_HPP

#include <iol/tag_invoke.hpp>

#include <concepts>
#include <exception>
#include <type_traits>

namespace iol
{

namespace execution
{

namespace _receiver
{

struct set_value_t
{

  template <typename Receiver, typename... Args>
    requires tag_invocable<set_value_t, Receiver, Args...>
  constexpr void operator()(Receiver&& receiver, Args&&... args) const
      noexcept(nothrow_tag_invocable<set_value_t, Receiver, Args...>)
  {
    static_assert(nothrow_tag_invocable<set_value_t, Receiver, Args...>);
    tag_invoke(set_value_t{}, (Receiver &&) receiver, (Args &&) args...);
  }
};

struct set_error_t
{

  template <typename Receiver, typename Error>
    requires tag_invocable<set_error_t, Receiver, Error>
  constexpr void operator()(Receiver&& receiver, Error&& error) const
      noexcept(nothrow_tag_invocable<set_error_t, Receiver, Error>)
  {
    static_assert(nothrow_tag_invocable<set_error_t, Receiver, Error>);
    tag_invoke(set_error_t{}, (Receiver &&) receiver, (Error &&) error);
  }
};

struct set_stopped_t
{

  template <typename Receiver>
    requires tag_invocable<set_stopped_t, Receiver>
  constexpr void operator()(Receiver&& receiver) const
      noexcept(nothrow_tag_invocable<set_stopped_t, Receiver>)
  {
    static_assert(nothrow_tag_invocable<set_stopped_t, Receiver>);
    tag_invoke(set_stopped_t{}, (Receiver &&) receiver);
  }
};

template <typename T>
concept receiver_tag =
    (std::same_as<T, set_value_t> || std::same_as<T, set_error_t> ||
     std::same_as<T, set_stopped_t>);

}  // namespace _receiver

using _receiver::set_value_t;

using _receiver::set_error_t;

using _receiver::set_stopped_t;

inline constexpr set_value_t set_value{};

inline constexpr set_error_t set_error{};

inline constexpr set_stopped_t set_stopped{};

template <typename T, typename E = std::exception_ptr>
concept receiver = std::move_constructible<std::remove_cvref_t<T>> &&
    std::constructible_from<std::remove_cvref_t<T>, T> &&
    requires(std::remove_cvref_t<T>&& t, E&& e)
{
  {
    execution::set_stopped(std::move(t))
  }
  noexcept;
  {
    execution::set_error(std::move(t), (E &&) e)
  }
  noexcept;
};

template <typename T, typename... Args>
concept receiver_of = receiver<T> && requires(std::remove_cvref_t<T>&& t, Args&&... args)
{
  {
    execution::set_value(std::move(t), (Args &&) args...)
  }
  noexcept;
};

namespace _receiver
{

struct forwarding_receiver_query_t
{

  template <typename T>
  constexpr bool operator()(T&& t) const noexcept
  {
    if constexpr (tag_invocable<forwarding_receiver_query_t, T>) {
      if constexpr (std::convertible_to<
                        tag_invoke_result_t<forwarding_receiver_query_t, T>, bool>) {
        static_assert(nothrow_tag_invocable<forwarding_receiver_query_t, T>);
        return tag_invoke(*this, (T &&) t);
      } else if constexpr (receiver_tag<T>) {
        return false;
      } else {
        return true;
      }
    } else if constexpr (receiver_tag<T>) {
      return false;
    } else {
      return true;
    }
  }
};

}  // namespace _receiver

using _receiver::forwarding_receiver_query_t;

inline constexpr forwarding_receiver_query_t forwarding_receiver_query{};

template <typename T>
concept is_forwarding_receiver_query = forwarding_receiver_query(T{});

}  // namespace execution

}  // namespace iol

#endif  // IOL_EXECUTION_RECEIVER_HPP
