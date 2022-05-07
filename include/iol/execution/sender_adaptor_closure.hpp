#ifndef IOL_EXECUTION_SENDER_ADAPTOR_CLOSURE_HPP
#define IOL_EXECUTION_SENDER_ADAPTOR_CLOSURE_HPP

#include <iol/type_traits.hpp>

#include <iol/execution/sender.hpp>

#include <type_traits>
#include <concepts>
#include <tuple>

namespace iol::execution
{

namespace _sender_adaptor_closure
{

namespace closure
{

template <typename>
struct sender_adaptor_closure
{};

}  // namespace closure

template <typename T>
concept sender_adaptor_closure_object =
    std::derived_from<
        std::remove_cvref_t<T>, closure::sender_adaptor_closure<std::remove_cvref_t<T>>> &&
    !sender<T> &&
    std::move_constructible<T>;

template <typename CPO, typename CPO2>
struct compose : closure::sender_adaptor_closure<compose<CPO, CPO2>>
{
  [[no_unique_address]] std::decay_t<CPO>  cpo_;
  [[no_unique_address]] std::decay_t<CPO2> cpo2_;

  template <typename Arg>
  constexpr decltype(auto) operator()(Arg&& arg) const&
  {
    return cpo_(cpo2_((Arg &&) arg));
  }

  template <typename Arg>
  constexpr decltype(auto) operator()(Arg&& arg) &&
  {
    return std::move(cpo_)(std::move(cpo2_)((Arg &&) arg));
  }
};

template <sender_adaptor_closure_object CPO1, sender_adaptor_closure_object CPO2>
constexpr compose<CPO2, CPO1> operator|(CPO1&& cpo1, CPO2&& cpo2)
{
  return {{}, (CPO2 &&) cpo2, (CPO1 &&) cpo1};
}

template <sender S, sender_adaptor_closure_object CPO>
// requires is_callable_v<CPO&&, S&&>
constexpr auto operator|(S&& sender, CPO&& cpo)
{
  return ((CPO &&) cpo)((S &&) sender);
}

template <typename CPO, typename... Args>
struct bind_back : closure::sender_adaptor_closure<bind_back<CPO, Args...>>
{

  [[no_unique_address]] CPO cpo_;
  std::tuple<Args...>       args_;

  template <typename S>
  constexpr decltype(auto) operator()(S&& s) const&
  {
    return std::apply([&](auto const&... ts) { return cpo_((S &&) s, ts...); }, args_);
  }

  template <typename S>
  constexpr decltype(auto) operator()(S&& s) &&
  {
    return std::apply(
        [&](auto&... ts) { return std::move(cpo_)((S &&) s, std::move(ts)...); }, args_);
  }
};

template <typename CPO, typename Arg>
struct bind_back<CPO, Arg> : closure::sender_adaptor_closure<bind_back<CPO, Arg>>
{

  [[no_unique_address]] CPO cpo_;
  [[no_unique_address]] Arg arg_;

  template <typename S>
  constexpr decltype(auto) operator()(S&& s) const&
  {
    return cpo_((S &&) s, arg_);
  }

  template <typename S>
  constexpr decltype(auto) operator()(S&& s) &&
  {
    return std::move(cpo_)((S &&) s, std::move(arg_));
  };
};

};  // namespace _sender_adaptor_closure

template <typename CPO, typename... Args>
using sender_adaptor_closure = _sender_adaptor_closure::bind_back<CPO, Args...>;

}  // namespace iol::execution

#endif  // IOL_EXECUTION_SENDER_ADAPTOR_CLOSURE_HPP
