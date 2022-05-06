#ifndef IOL_META_HPP
#define IOL_META_HPP

#include <type_traits>
#include <concepts>

namespace iol::meta
{

template <typename T>
using t_ = typename T::type;

template <typename... Ts>
struct m_list
{
  using type = m_list;
  static constexpr std::size_t size = sizeof...(Ts);
};

template <typename T>
using m_identity = std::type_identity<T>;

template <typename T>
using m_identity_t = t_<m_identity<T>>;

template <typename... Ts>
struct m_inherit : Ts...
{};

template <std::size_t I>
using m_size_t = std::integral_constant<std::size_t, I>;

template <bool V>
using m_bool = std::bool_constant<V>;

using m_true = m_bool<true>;

using m_false = m_bool<false>;

template <typename T>
inline static constexpr auto m_value = T::value;

namespace detail
{

template <typename L>
struct m_size_impl;

template <template <typename...> class List, typename... Ts>
struct m_size_impl<List<Ts...>>
{
  using type = m_size_t<sizeof...(Ts)>;
};

}  // namespace detail

template <typename L>
using m_size = t_<detail::m_size_impl<L>>;

namespace detail
{

template <typename L, typename...>
struct m_push_back_impl
{};

template <template <typename...> class L, typename... Ts, typename... Vs>
struct m_push_back_impl<L<Ts...>, Vs...>
{
  using type = L<Ts..., Vs...>;
};

template <typename L, typename...>
struct m_push_front_impl
{};

template <template <typename...> class L, typename... Ts, typename... Vs>
struct m_push_front_impl<L<Ts...>, Vs...>
{
  using type = L<Vs..., Ts...>;
};

template <typename L>
struct m_clear_impl
{};

template <template <typename...> class L, typename... Ts>
struct m_clear_impl<L<Ts...>>
{
  using type = L<>;
};

}  // namespace detail

template <typename L, typename... Ts>
using m_push_back = t_<detail::m_push_back_impl<L, Ts...>>;

template <typename L, typename... Ts>
using m_push_front = t_<detail::m_push_front_impl<L, Ts...>>;

template <typename L>
using m_clear = t_<detail::m_clear_impl<L>>;

namespace detail
{

template <typename L, typename L2>
struct m_assign_impl
{};

template <
    template <typename...> class L, typename... Ts, template <typename...> class L2, typename... Us>
struct m_assign_impl<L<Ts...>, L2<Us...>>
{
  using type = L<Us...>;
};

}  // namespace detail

template <typename L, typename L2>
using m_assign = t_<detail::m_assign_impl<L, L2>>;

namespace detail
{

template <typename L>
struct m_first_impl
{};

template <template <typename...> class L, typename T1, typename... Ts>
struct m_first_impl<L<T1, Ts...>>
{
  using type = T1;
};

template <typename L>
struct m_second_impl
{};

template <template <typename...> class L, typename T1, typename T2, typename... Ts>
struct m_second_impl<L<T1, T2, Ts...>>
{
  using type = T2;
};

template <typename L>
struct m_third_impl
{};

template <template <typename...> class L, typename T1, typename T2, typename T3, typename... Ts>
struct m_third_impl<L<T1, T2, T3, Ts...>>
{
  using type = T3;
};

}  // namespace detail

template <typename L>
using m_first = t_<detail::m_first_impl<L>>;

template <typename L>
using m_second = t_<detail::m_second_impl<L>>;

template <typename L>
using m_third = t_<detail::m_third_impl<L>>;

namespace detail
{

template <template <typename...> class Fn, typename... Ts>
struct m_valid_impl
{
  using type = m_false;
};

template <template <typename...> class Fn, typename... Ts>
  requires requires
  {
    typename Fn<Ts...>;
  }
struct m_valid_impl<Fn, Ts...>
{
  using type = m_true;
};

}  // namespace detail

template <template <typename...> class Fn, typename... Ts>
using m_valid = t_<detail::m_valid_impl<Fn, Ts...>>;

namespace detail
{

template <bool If, typename True, typename...>
struct m_if_impl
{};

template <typename True, typename... False>
struct m_if_impl<true, True, False...>
{
  using type = True;
};

template <typename True, typename False>
struct m_if_impl<false, True, False>
{
  using type = False;
};

}  // namespace detail

template <bool If, typename T, typename... F>
using m_if_c = t_<detail::m_if_impl<If, T, F...>>;

template <typename If, typename T, typename... F>
using m_if = m_if_c<m_value<If>, T, F...>;

template <typename T>
using m_to_bool = m_bool<bool(m_value<T>)>;

namespace detail
{

template <typename Q, typename... Ts>
struct m_valid_q_impl
{
  using type = m_false;
};

template <typename Q, typename... Ts>
  requires requires
  {
    typename m_valid<Q::template fn, Ts...>;
  } && m_value<m_valid<Q::template fn, Ts...>>
struct m_valid_q_impl<Q, Ts...>
{
  using type = m_true;
};

}  // namespace detail

template <typename Q, typename... Ts>
using m_valid_q = t_<detail::m_valid_q_impl<Q, Ts...>>;

template <template <typename...> class Fn, typename... Args>
struct m_defer
{};

template <template <typename...> class Fn, typename... Args>
  requires m_value<m_valid<Fn, Args...>>
struct m_defer<Fn, Args...>
{
  using type = Fn<Args...>;
};

template <typename Q, typename... Args>
using m_defer_q = m_defer<Q::template fn, Args...>;

template <template <typename...> class F>
struct m_quote
{
  template <typename... Args>
  using fn = t_<m_defer<F, Args...>>;
};

template <template <typename> class F>
struct m_quote1
{
  template <typename Arg>
  using fn = F<Arg>;
};

template <template <typename, typename> class F>
struct m_quote2
{
  template <typename Arg1, typename Arg2>
  using fn = F<Arg1, Arg2>;
};

template <template <typename, typename, typename> class F>
struct m_quote3
{
  template <typename Arg1, typename Arg2, typename Arg3>
  using fn = F<Arg1, Arg2, Arg3>;
};

template <typename Q, typename... Args>
using m_invoke_q = t_<m_defer_q<Q, Args...>>;

template <typename T>
using m_not = m_bool<!m_value<m_to_bool<T>>>;

template <template <typename...> class Fn>
struct m_not_fn
{
  template <typename... Args>
  using fn = m_not<t_<m_defer<Fn, Args...>>>;
};

template <typename Q>
using m_not_fn_q = m_not_fn<Q::template fn>;

namespace detail
{

template <typename...>
struct m_same_impl
{
  using type = m_false;
};

template <typename T, typename... Ts>
  requires(std::same_as<T, Ts>&&...)
struct m_same_impl<T, Ts...>
{
  using type = m_true;
};

template <typename T>
struct m_same_impl<T>
{
  using type = m_true;
};

template <>
struct m_same_impl<>
{
  using type = m_true;
};

}  // namespace detail

template <typename... Ts>
using m_same = t_<detail::m_same_impl<Ts...>>;

// does not perform short-circuit evaluation
template <typename... Ts>
using m_all = m_bool<(m_value<m_to_bool<Ts>> && ...)>;

template <typename... Ts>
using m_any = m_bool<(m_value<m_to_bool<Ts>> || ...)>;

namespace detail
{

template <typename...>
struct m_and_impl
{
  using type = m_false;
};

template <>
struct m_and_impl<>
{
  using type = m_true;
};

template <typename T, typename... Ts>
  requires m_value<m_valid<m_to_bool, T>> && m_value<m_to_bool<T>>
struct m_and_impl<T, Ts...>
{
  using type = t_<m_and_impl<Ts...>>;
};

template <typename...>
struct m_or_impl;

template <>
struct m_or_impl<>
{
  using type = m_false;
};

template <typename T>
struct m_or_impl<T>
{
  using type = m_to_bool<T>;
};

template <typename T, typename... Ts>
struct m_or_impl<T, Ts...>
{
  using type = t_<m_if<m_to_bool<T>, m_true, m_or_impl<Ts...>>>;
};

}  // namespace detail

// performs short-circuit evaluation
template <typename... Ts>
using m_and = t_<detail::m_and_impl<Ts...>>;

template <typename... Ts>
using m_or = t_<detail::m_or_impl<Ts...>>;

namespace detail
{

template <typename, template <typename...> class N>
struct m_rename_impl
{};

template <template <typename...> class L, typename... Ts, template <typename...> class N>
struct m_rename_impl<L<Ts...>, N>
{
  using type = N<Ts...>;
};

}  // namespace detail

template <typename L, template <typename...> class N>
using m_rename = t_<detail::m_rename_impl<L, N>>;

template <template <typename...> class F, typename L>
using m_apply = t_<detail::m_rename_impl<L, F>>;

template <typename Q, typename L>
using m_apply_q = t_<detail::m_rename_impl<L, Q::template fn>>;

namespace detail
{

template <typename... Ls>
struct m_concat_impl
{};

}  // namespace detail

template <typename... Ls>
using m_concat = t_<detail::m_concat_impl<Ls...>>;

namespace detail
{

template <template <typename...> class L1, typename... T1s>
struct m_concat_impl<L1<T1s...>>
{
  using type = L1<T1s...>;
};

template <
    template <typename...> class L1, typename... T1s, template <typename...> class L2,
    typename... T2s>
struct m_concat_impl<L1<T1s...>, L2<T2s...>>
{
  using type = L1<T1s..., T2s...>;
};

template <
    template <typename...> class L1, typename... T1s, template <typename...> class L2,
    typename... T2s, template <typename...> class L3, typename... T3s>
struct m_concat_impl<L1<T1s...>, L2<T2s...>, L3<T3s...>>
{
  using type = L1<T1s..., T2s..., T3s...>;
};

template <
    template <typename...> class L1, typename... T1s, template <typename...> class L2,
    typename... T2s, template <typename...> class L3, typename... T3s,
    template <typename...> class L4, typename... T4s>
struct m_concat_impl<L1<T1s...>, L2<T2s...>, L3<T3s...>, L4<T4s...>>
{
  using type = L1<T1s..., T2s..., T3s..., T4s...>;
};

template <
    template <typename...> class L1, typename... T1s, template <typename...> class L2,
    typename... T2s, template <typename...> class L3, typename... T3s,
    template <typename...> class L4, typename... T4s, template <typename...> class L5,
    typename... T5s>
struct m_concat_impl<L1<T1s...>, L2<T2s...>, L3<T3s...>, L4<T4s...>, L5<T5s...>>
{
  using type = L1<T1s..., T2s..., T3s..., T4s..., T5s...>;
};

template <
    template <typename...> class L1, typename... T1s, template <typename...> class L2,
    typename... T2s, template <typename...> class L3, typename... T3s,
    template <typename...> class L4, typename... T4s, template <typename...> class L5,
    typename... T5s, template <typename...> class L6, typename... T6s, typename... Ls>
struct m_concat_impl<L1<T1s...>, L2<T2s...>, L3<T3s...>, L4<T4s...>, L5<T5s...>, L6<T6s...>, Ls...>
{
  using type = m_concat<L1<T1s..., T2s..., T3s..., T4s..., T5s..., T6s...>, Ls...>;
};

}  // namespace detail

template <template <typename...> class F, typename... Ts>
struct m_bind_back
{
  template <typename... Args>
  using fn = t_<m_defer<F, Args..., Ts...>>;
};

template <template <typename...> class F, typename... Ts>
struct m_bind_front
{
  template <typename... Args>
  using fn = t_<m_defer<F, Ts..., Args...>>;
};

template<typename Q, typename ... Ts>
using m_bind_back_q = m_bind_back<Q::template fn, Ts...>;

template<typename Q, typename ... Ts>
using m_bind_front_q = m_bind_front<Q::template fn, Ts...>;

namespace detail
{

template <typename L, typename V, template <typename...> class F>
struct m_fold_impl
{};

template <template <typename...> class L, typename T, typename V, template <typename...> class F>
struct m_fold_impl<L<T>, V, F>
{
  using type = F<V, T>;
};

template <
    template <typename...> class L, typename T1, typename T2, typename V,
    template <typename...> class F>
struct m_fold_impl<L<T1, T2>, V, F>
{
  using type = F<F<V, T1>, T2>;
};

template <
    template <typename...> class L, typename T1, typename T2, typename T3, typename V,
    template <typename...> class F>
struct m_fold_impl<L<T1, T2, T3>, V, F>
{
  using type = F<F<F<V, T1>, T2>, T3>;
};

template <
    template <typename...> class L, typename T1, typename T2, typename T3, typename T4, typename V,
    template <typename...> class F>
struct m_fold_impl<L<T1, T2, T3, T4>, V, F>
{
  using type = F<F<F<F<V, T1>, T2>, T3>, T4>;
};

template <
    template <typename...> class L, typename T1, typename T2, typename T3, typename T4, typename T5,
    typename... Ts, typename V, template <typename...> class F>
struct m_fold_impl<L<T1, T2, T3, T4, T5, Ts...>, V, F>
{
  using type = t_<m_fold_impl<L<Ts...>, F<F<F<F<F<V, T1>, T2>, T3>, T4>, T5>, F>>;
};

template <
    template <typename...> class L, typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6, typename T7, typename T8, typename T9, typename T10, typename... Ts, typename V,
    template <typename...> class F>
struct m_fold_impl<L<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, Ts...>, V, F>
{
  using type = t_<m_fold_impl<
      L<Ts...>, F<F<F<F<F<F<F<F<F<F<V, T1>, T2>, T3>, T4>, T5>, T6>, T7>, T8>, T9>, T10>, F>>;
};

template <template <typename...> class L, typename V, template <typename...> class F>
struct m_fold_impl<L<>, V, F>
{
  using type = V;
};

}  // namespace detail

template <typename L, typename V, template <typename...> class F>
using m_fold = t_<detail::m_fold_impl<L, V, F>>;

template <typename L, typename V, typename Q>
using m_fold_q = t_<detail::m_fold_impl<L, V, Q::template fn>>;

namespace detail
{

struct m_invalid_transform_tag
{};

template <template <typename...> class F, typename... Ls>
struct m_transform_impl
{};

}  // namespace detail

template <template <typename...> class F, typename... Ls>
using m_transform = t_<m_if<
    m_same<m_size<Ls>...>, detail::m_transform_impl<F, Ls...>, detail::m_invalid_transform_tag>>;

template <typename Q, typename... Ls>
using m_transform_q = m_transform<Q::template fn, Ls...>;

namespace detail
{

template <template <typename...> class F, template <typename...> class L1, typename... T1s>
struct m_transform_impl<F, L1<T1s...>>
{
  using type = L1<F<T1s>...>;
};

template <
    template <typename...> class F, template <typename...> class L1, typename... T1s,
    template <typename...> class L2, typename... T2s>
struct m_transform_impl<F, L1<T1s...>, L2<T2s...>>
{
  using type = L1<F<T1s, T2s>...>;
};

template <
    template <typename...> class F, template <typename...> class L1, typename... T1s,
    template <typename...> class L2, typename... T2s, template <typename...> class L3,
    typename... T3s>
struct m_transform_impl<F, L1<T1s...>, L2<T2s...>, L3<T3s...>>
{
  using type = L1<F<T1s, T2s, T3s>...>;
};

template <
    template <typename...> class F, template <typename...> class L1, typename... T1s,
    template <typename...> class L2, typename... T2s, template <typename...> class L3,
    typename... T3s, template <typename...> class L4, typename... T4s>
struct m_transform_impl<F, L1<T1s...>, L2<T2s...>, L3<T3s...>, L4<T4s...>>
{
  using type = L1<F<T1s, T2s, T3s, T4s>...>;
};

template <
    template <typename...> class F, template <typename...> class L1, typename... T1s,
    template <typename...> class L2, typename... T2s, template <typename...> class L3,
    typename... T3s, template <typename...> class L4, typename... T4s,
    template <typename...> class L5, typename... T5s>
struct m_transform_impl<F, L1<T1s...>, L2<T2s...>, L3<T3s...>, L4<T4s...>, L5<T5s...>>
{
  using type = L1<F<T1s, T2s, T3s, T4s, T5s>...>;
};

template <
    template <typename...> class F, template <typename...> class L1, typename... T1s,
    template <typename...> class L2, typename... T2s, template <typename...> class L3,
    typename... T3s, template <typename...> class L4, typename... T4s,
    template <typename...> class L5, typename... T5s, typename... Ls>
struct m_transform_impl<F, L1<T1s...>, L2<T2s...>, L3<T3s...>, L4<T4s...>, L5<T5s...>, Ls...>
{
  using R1 = L1<m_list<T1s, T2s, T3s, T4s, T5s>...>;

  template <typename L, typename V>
  using F1 = m_transform<m_push_back, L, V>;

  using R2 = m_fold<m_list<Ls...>, R1, F1>;

  template <typename L>
  using F2 = m_apply<F, L>;

  using type = m_transform<F2, R2>;
};

template <template <typename...> class Fn, typename Qf, typename... Ls>
struct m_transform_if_impl
{
  template <typename T, typename... Ts>
  using F = t_<m_if<m_invoke_q<Qf, T, Ts...>, m_defer<Fn, T, Ts...>, m_identity<T>>>;

  using type = m_transform<F, Ls...>;
};

template <typename Q, typename...>
struct m_filter_impl
{};

template <typename Q, typename L, typename... Ls>
struct m_filter_impl<Q, L, Ls...>
{
  template <typename T, typename... Ts>
  using F1 = m_if<m_invoke_q<Q, T, Ts...>, m_list<T>, m_list<>>;

  using R1 = m_transform<F1, L, Ls...>;

  using type = m_apply<m_concat, R1>;
};

}  // namespace detail

template <template <typename...> class T, template <typename...> class F, typename... Ls>
using m_transform_if = t_<detail::m_transform_if_impl<T, m_quote<F>, Ls...>>;

template <typename Qt, typename Qf, typename... Ls>
using m_transform_if_q = t_<detail::m_transform_if_impl<Qt::template fn, Qf, Ls...>>;

template <template <typename...> class F, typename... Ls>
using m_filter = t_<detail::m_filter_impl<m_quote<F>, Ls...>>;

template <typename Q, typename... Ls>
using m_filter_q = t_<detail::m_filter_impl<Q, Ls...>>;

namespace detail
{

template <typename R, typename L>
struct m_unique_impl;

template <typename R, template <typename...> class L, typename T, typename... Ts>
struct m_unique_impl<R, L<T, Ts...>>
{
  using IL = m_apply<m_inherit, m_transform<m_identity, R>>;

  using R1 = m_if<std::is_base_of<m_identity<T>, IL>, R, m_push_back<R, T>>;

  using type = t_<m_unique_impl<R1, L<Ts...>>>;
};

template <typename R, template <typename...> class L>
struct m_unique_impl<R, L<>>
{
  using type = R;
};

}  // namespace detail

template <typename L>
using m_unique = t_<detail::m_unique_impl<m_clear<L>, L>>;

namespace detail
{

struct type_counter
{
  std::size_t n;
  constexpr   operator std::size_t() noexcept { return n; }
};

template <typename C>
constexpr type_counter operator+(type_counter c, C)
{
  if constexpr (m_value<m_to_bool<C>>)
    return {c.n + 1};
  else
    return c;
}

template <typename L, typename Q>
struct m_count_impl
{};

template <template <typename...> class L, typename... Ts, typename Q>
struct m_count_impl<L<Ts...>, Q>
{
  static constexpr auto counter = type_counter{0};
  using type = m_size_t<(counter + ... + m_invoke_q<Q, Ts>{})>;
};

}  // namespace detail

template <typename L, typename V>
using m_count = t_<detail::m_count_impl<L, m_bind_back<m_same, V>>>;

template <typename L, template <typename> class P>
using m_count_if = t_<detail::m_count_impl<L, m_quote<P>>>;

template <typename L, typename Q>
using m_count_if_q = t_<detail::m_count_impl<L, Q>>;

template <typename L, typename V>
using m_contains = m_to_bool<m_count<L, V>>;

}  // namespace iol::meta

#endif  // IOL_META_HPP
