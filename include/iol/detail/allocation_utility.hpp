#ifndef IOL_DETAIL_ALLOCATION_UTILITY_HPP
#define IOL_DETAIL_ALLOCATION_UTILITY_HPP

#include <iol/detail/operation_base.hpp>
#include <memory>
#include <type_traits>

namespace iol::detail::allocation_utility
{

template <typename T, typename Alloc, typename... Args>
requires std::is_constructible_v<T, Alloc, Args...>
inline operation_ptr make_operation(Alloc&& alloc, Args&&... args)
{

  using allocator =
      typename std::allocator_traits<std::remove_cvref_t<Alloc>>::template rebind_alloc<T>;
  using allocator_traits = std::allocator_traits<allocator>;

  allocator rebound_alloc{alloc};

  T* mem = allocator_traits::allocate(rebound_alloc, 1);

  if constexpr (std::is_nothrow_constructible_v<T, Alloc, Args...>) {
    allocator_traits::construct(
        rebound_alloc, mem, std::forward<Alloc>(alloc), std::forward<Args>(args)...);
  } else {
    try {
      allocator_traits::construct(
          rebound_alloc, mem, std::forward<Alloc>(alloc), std::forward<Args>(args)...);
    } catch (...) {
      allocator_traits::deallocate(rebound_alloc, mem, 1);
      throw;
    }
  }

  return operation_ptr{mem};
}

template <typename Alloc, typename T>
inline constexpr void destroy_and_delete(Alloc& alloc, T* t) noexcept
{

  using allocator =
      typename std::allocator_traits<std::remove_cvref_t<Alloc>>::template rebind_alloc<T>;
  using allocator_traits = std::allocator_traits<allocator>;

  allocator rebound_alloc(alloc);

  allocator_traits::destroy(rebound_alloc, t);
  allocator_traits::deallocate(rebound_alloc, t, 1);
}

}  // namespace iol::detail::allocation_utility

#endif  // IOL_DETAIL_ALLOCATION_UTILITY_HPP
