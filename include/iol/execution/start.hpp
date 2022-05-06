#ifndef IOL_EXECUTION_START_HPP
#define IOL_EXECUTION_START_HPP

#include <concepts>
#include <iol/tag_invoke.hpp>
#include <type_traits>

namespace iol::execution
{

namespace start_impl
{

struct start_t
{
  template <typename O>
    requires tag_invocable<start_t, O&>
  decltype(auto) operator()(O& o) const noexcept(nothrow_tag_invocable<start_t, O&>)
  {
    static_assert(nothrow_tag_invocable<start_t, O&>);
    return tag_invoke(start_t{}, o);
  }
};

}  // namespace start_impl

using start_impl::start_t;

inline constexpr start_t start{};

template <typename O>
concept operation_state = std::destructible<O> && std::is_object_v<O> && requires(O& o)
{
  {
    start(o)
  }
  noexcept;
};

}  // namespace iol::execution

#endif  // IOL_EXECUTION_START_HPP
