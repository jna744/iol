#ifndef IOL_SYNC_WAIT_HPP
#define IOL_SYNC_WAIT_HPP

#include <iol/awaitable_traits.hpp>
#include <iol/detail/simple_manual_reset_event.hpp>
#include <iol/detail/sync_wait_task.hpp>

namespace iol
{

template <typename Awaitable>
requires requires
{
  typename awaitable_result_t<Awaitable&&>;
}
auto sync_wait(Awaitable&& awaitable) -> awaitable_result_t<Awaitable&&>
{
  detail::simple_manual_reset_event event;
  auto sync_task = detail::make_sync_wait_task<awaitable_result_t<Awaitable&&>>(
      std::forward<Awaitable>(awaitable));
  sync_task.start(&event);
  event.wait();
  return sync_task.get_value();
}

}  // namespace iol

#endif  // IOL_SYNC_WAIT_HPP
