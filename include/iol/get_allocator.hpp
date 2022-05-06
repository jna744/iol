#ifndef IOL_GET_ALLOCATOR_HPP
#define IOL_GET_ALLOCATOR_HPP

#include <concepts>
#include <iol/tag_invoke.hpp>
#include <memory>
#include <utility>

namespace iol
{

namespace get_allocator_impl
{

struct dummy {
};

template <typename Alloc>
using dummy_allocator_t = typename std::allocator_traits<Alloc>::template rebind_alloc<dummy>;

template <typename T>
concept valid_allocator = std::default_initializable<T> && std::copy_constructible<T> &&
    requires(T &t, typename std::allocator_traits<T>::size_type n)
{
  typename std::allocator_traits<T>::value_type;
  typename dummy_allocator_t<T>;
  requires std::constructible_from<dummy_allocator_t<T>, T &>;
  requires(std::same_as<void, typename std::allocator_traits<T>::value_type> || requires {
    typename std::allocator_traits<T>::pointer;
    {
      t.allocate(n)
      } -> std::same_as<typename std::allocator_traits<T>::pointer>;
    t.deallocate(n);
  });
};

}  // namespace get_allocator_impl

inline constexpr struct get_allocator_t {

  template <typename T>
  constexpr auto operator()(T &&t) const noexcept
  {
    using TT = std::remove_cvref_t<T>;
    if constexpr (tag_invocable<get_allocator_t, TT const &>) {
      static_assert(nothrow_tag_invocable<get_allocator_t, TT const &>);
      static_assert(
          get_allocator_impl::valid_allocator<tag_invoke_result_t<get_allocator_t, TT const &>>);
      return tag_invoke(get_allocator_t{}, std::as_const(t));
    } else {
      return std::allocator<void>{};
    }
  }

} get_allocator;

template <typename T>
using allocator_t = decltype(get_allocator(std::declval<std::remove_cvref_t<T> const &>()));

}  // namespace iol

#endif  // IOL_GET_ALLOCATOR_HPP
