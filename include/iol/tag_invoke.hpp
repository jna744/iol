#ifndef IOL_TAG_INVOKE_HPP
#define IOL_TAG_INVOKE_HPP

#include <type_traits>

namespace iol
{

namespace tag_invoke_impl
{

void tag_invoke();

template <typename Tag, typename... Args>
concept tag_invocable = requires(Tag&& tag, Args&&... args)
{
  tag_invoke((Tag &&) tag, (Args &&) args...);
};

template <typename Tag, typename... Args>
concept nothrow_tag_invocable = tag_invocable<Tag, Args...> && requires(Tag&& tag, Args&&... args)
{
  {
    tag_invoke((Tag &&) tag, (Args &&) args...)
  }
  noexcept;
};

template <typename Tag, typename... Args>
using tag_invoke_result_t = decltype(tag_invoke(std::declval<Tag>(), std::declval<Args>()...));

template <typename Tag, typename... Args>
struct tag_invoke_result {
};

template <typename Tag, typename... Args>
requires tag_invocable<Tag, Args...>
struct tag_invoke_result<Tag, Args...> {
  using type = tag_invoke_result_t<Tag, Args...>;
};

struct tag_invoke_t {

  template <typename Tag, typename... Args>
  requires tag_invocable<Tag, Args...>
  constexpr auto operator()(Tag&& tag, Args&&... args) const noexcept(nothrow_tag_invocable<Tag, Args...>)
      -> tag_invoke_result_t<Tag, Args...>
  {
    return tag_invoke((Tag &&) tag, (Args &&) args...);
  }
};

}  // namespace tag_invoke_impl

using tag_invoke_impl::tag_invocable;

using tag_invoke_impl::nothrow_tag_invocable;

using tag_invoke_impl::tag_invoke_result;

using tag_invoke_impl::tag_invoke_result_t;

inline constexpr tag_invoke_impl::tag_invoke_t tag_invoke{};

template <auto& Tag>
using tag_t = std::decay_t<decltype(Tag)>;

}  // namespace iol

#endif  // IOL_TAG_INVOKE_HPP
