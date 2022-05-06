#ifndef IOL_EXECUTION_COMPLETION_SIGNATURES_HPP
#define IOL_EXECUTION_COMPLETION_SIGNATURES_HPP

#include <iol/execution/env.hpp>
#include <iol/execution/receiver.hpp>
#include <iol/execution/sender.hpp>

#include <iol/is_awaitable.hpp>
#include <iol/awaitable_traits.hpp>
#include <iol/meta.hpp>

#include <concepts>
#include <exception>
#include <type_traits>
#include <variant>

namespace iol::execution
{

namespace completion_signatures_impl
{

template <typename T>
struct signature
{};

template <typename Tag>
consteval bool validate_tag_args(std::size_t arg_count)
{
  using meta::m_same;
  if (m_same<Tag, set_error_t>{})
    return arg_count == 1;
  if (m_same<Tag, set_stopped_t>{})
    return arg_count == 0;
  return true;
}

template <receiver_impl::receiver_tag Tag, typename... Vs>
struct signature<Tag(Vs...)>
{
  static_assert(validate_tag_args<Tag>(sizeof...(Vs)), "completion signature arg mismatch");

  using tag_t = Tag;
  using args_t = meta::t_<meta::m_if<
      meta::m_same<Tag, set_error_t>, meta::m_defer<meta::m_first, meta::m_list<Vs...>>,
      meta::m_list<Vs...>>>;
};

template <typename S, typename... Args>
struct signature<S (*)(Args...)> : signature<S(Args...)>
{};

}  // namespace completion_signatures_impl

template <typename Fn>
concept completion_signature = requires
{
  typename completion_signatures_impl::signature<Fn>::tag_t;
};

template <completion_signature... Fns>
struct completion_signatures
{

 private:

  template <typename Tag, typename T>
  using equal_tag = meta::m_same<Tag, typename completion_signatures_impl::signature<T>::tag_t>;

  template <typename Tag>
  using tag_sigs_t = meta::m_filter_q<meta::m_bind_front<equal_tag, Tag>, meta::m_list<Fns...>>;

  template <typename Signature>
  using tag_args_t = typename completion_signatures_impl::signature<Signature>::args_t;

  template <typename QTuple, typename Signature>
  using value_types_helper = meta::m_apply_q<QTuple, tag_args_t<Signature>>;

 public:

  template <template <class...> class Tuple, template <class...> class Variant>
  using value_types = meta::m_apply<
      Variant,
      meta::m_transform_q<
          meta::m_bind_front<value_types_helper, meta::m_quote<Tuple>>, tag_sigs_t<set_value_t>>>;

  template <template <typename...> class Variant>
  using error_types =
      meta::m_apply<Variant, meta::m_transform<tag_args_t, tag_sigs_t<set_error_t>>>;

  static constexpr bool sends_stopped = meta::m_any<equal_tag<set_stopped_t, Fns>...>{};
};

namespace completion_signatures_impl
{

struct no_completion_signatures
{};

template <typename T>
concept with_completion_signatures = requires
{
  typename std::remove_cvref_t<T>::completion_signatures;
};

struct get_completion_signatures_t
{

  template <typename S, typename E>
  constexpr auto operator()(S &&s, E &&e) const
  {
    if constexpr (tag_invocable<get_completion_signatures_t, S, E>) {
      using signatures_t = tag_invoke_result_t<get_completion_signatures_t, S, E>;
      return signatures_t{};
    } else if constexpr (with_completion_signatures<S>) {
      using signatures_t = typename std::remove_cvref_t<S>::completion_signatures;
      return signatures_t{};
    } else if constexpr (is_awaitable<S>) {
      if constexpr (std::same_as<awaitable_result_t<S>, void>) {
        return completion_signatures<
            set_value_t(), set_error_t(std::exception_ptr), set_stopped_t()>{};
      } else {
        return completion_signatures<
            set_value_t(awaitable_result_t<S>), set_error_t(std::exception_ptr), set_stopped_t()>{};
      }
    } else {
      return no_completion_signatures{};
    }
  }

  template <typename S>
  constexpr auto operator()(S &&s) const
  {
    return (*this)((S &&) s, no_env{});
  }
};

}  // namespace completion_signatures_impl

using completion_signatures_impl::get_completion_signatures_t;
using completion_signatures_impl::no_completion_signatures;

inline constexpr get_completion_signatures_t get_completion_signatures{};

struct empty_variant
{
  empty_variant() = delete;
};

namespace completion_signatures_impl
{

template <typename T, typename U>
concept not_same_as = !std::same_as<T, U>;

template <typename S, typename E>
struct completion_signatures_of_impl
{};

template <typename S, typename E>
  requires requires(S &&s, E &&e)
  {
    {
      get_completion_signatures((S &&) s, (E &&) e)
      } -> not_same_as<no_completion_signatures>;
  }
struct completion_signatures_of_impl<S, E>
{
  using type = decltype(get_completion_signatures(std::declval<S>(), std::declval<E>()));
};

}  // namespace completion_signatures_impl

template <typename S, typename E>
using completion_signatures_of_t =
    typename completion_signatures_impl::completion_signatures_of_impl<S, E>::type;

template <typename... Ts>
using decayed_tuple = std::tuple<std::decay_t<Ts>...>;

template <typename... Ts>
using variant_or_empty = meta::t_<meta::m_if_c<
    (sizeof...(Ts) > 0), meta::m_defer<std::variant, std::decay_t<Ts>...>,
    meta::m_identity<empty_variant>>>;

template <typename E>
struct dependent_completion_signatures
{};

template <>
struct dependent_completion_signatures<no_env>
{
  template <template <typename...> class, template <typename...> class>
    requires(false)
  using value_types = void;

  template <template <typename...> class>
    requires(false)
  using error_types = void;

  static constexpr bool sends_stopped = false;
};

template <
    typename S, typename E = no_env, template <typename...> class Tuple = decayed_tuple,
    template <typename...> class Variant = variant_or_empty>
  requires sender<S, E>
using value_types_of_t =
    typename completion_signatures_of_t<S, E>::template value_types<Tuple, Variant>;

template <typename S, typename E = no_env, template <typename...> class Variant = variant_or_empty>
  requires sender<S, E>
using error_types_of_t = typename completion_signatures_of_t<S, E>::template error_types<Variant>;

template <typename... Args>
using default_set_value = set_value_t(Args...);

template <typename Error>
using default_set_error = set_error_t(Error);

namespace completion_signatures_impl
{

template <
    typename S, typename E = no_env, typename Qt = meta::m_quote<decayed_tuple>,
    typename Qv = meta::m_quote<variant_or_empty>>
using value_types_of_t_q = value_types_of_t<S, E, Qt::template fn, Qv::template fn>;

template <typename S, typename E = no_env, typename Qv = meta::m_quote<variant_or_empty>>
using error_types_of_t_q = error_types_of_t<S, E, Qv::template fn>;

using list_q = meta::m_quote<meta::m_list>;

using not_void = meta::m_not_fn_q<meta::m_bind_front<meta::m_same, void>>;

template <typename T>
using is_valid_signature = meta::m_bool<completion_signature<T>>;

template <typename L>
using is_valid_signature_list =
    meta::m_apply<meta::m_all, meta::m_transform<is_valid_signature, L>>;

template <
    typename Sender, typename Env, typename AddlSigs, typename SetValueQ, typename SetErrorQ,
    bool SendsStopped>
struct make_completion_signatures_impl
{
  using type = dependent_completion_signatures<Env>;
};

template <
    typename Sender, typename Env, typename AddlSigs, typename SetValueQ, typename SetErrorQ,
    bool SendsStopped>
  requires meta::m_value<meta::m_all<
      meta::m_valid<value_types_of_t_q, Sender, Env, SetValueQ, list_q>,
      meta::m_valid<error_types_of_t_q, Sender, Env, list_q>>>
struct make_completion_signatures_impl<Sender, Env, AddlSigs, SetValueQ, SetErrorQ, SendsStopped>
{

  using value_types =
      meta::m_filter_q<not_void, value_types_of_t_q<Sender, Env, SetValueQ, list_q>>;

  // Invalid completion signatures detected
  static_assert(is_valid_signature_list<value_types>{}, "invalid set-value completion signature");

  using error_types = meta::m_transform_q<
      SetErrorQ, meta::m_filter_q<not_void, error_types_of_t_q<Sender, Env, list_q>>>;

  // Invalid completion signatures detected
  static_assert(is_valid_signature_list<error_types>{}, "invalid set-error completion signature");

  using type = meta::m_rename<
      meta::m_unique<meta::m_concat<
          value_types, error_types,
          meta::m_if_c<SendsStopped, meta::m_list<set_stopped_t()>, meta::m_list<>>, AddlSigs>>,
      completion_signatures>;
};

}  // namespace completion_signatures_impl

template <
    typename Sender, typename Env = no_env, typename AddlSigs = completion_signatures<>,
    template <typename...> class SetValue = default_set_value,
    template <typename> class SetError = default_set_error,
    bool SendsStopped = completion_signatures_of_t<Sender, Env>::sends_stopped>
  requires sender<Sender, Env>
using make_completion_signatures =
    typename completion_signatures_impl::make_completion_signatures_impl<
        Sender, Env, AddlSigs, meta::m_quote<SetValue>, meta::m_quote<SetError>,
        SendsStopped>::type;

}  // namespace iol::execution

#endif  // IOL_EXECUTION_COMPLETION_SIGNATURES_HPP
