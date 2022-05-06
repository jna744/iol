#ifndef IOL_DETAIL_SYNC_WAIT_TASK_HPP
#define IOL_DETAIL_SYNC_WAIT_TASK_HPP

#include <iol/detail/config.hpp>
#include <iol/detail/simple_manual_reset_event.hpp>

//

#include <coroutine>
#include <exception>
#include <type_traits>

namespace iol::detail
{

template <typename T>
class sync_wait_task;

template <typename T>
class sync_wait_task_promise;

template <typename T>
using sync_wait_coroutine_handle = std::coroutine_handle<sync_wait_task_promise<T>>;

struct sync_wait_notify_awaiter {

  bool await_ready() const noexcept { return false; }

  // This suspends execution of _this_ coroutine
  template <typename T>
  void await_suspend(sync_wait_coroutine_handle<T> handle) const noexcept
  {
    handle.promise().reset_event_->set();
  }

  void await_resume() const noexcept {}
};

template <typename T>
class sync_wait_task_promise
{

  friend sync_wait_notify_awaiter;

public:

  using coroutine_handle = std::coroutine_handle<sync_wait_task_promise<T>>;

  using value_type = std::remove_reference_t<T>;

  using reference = std::conditional_t<std::is_reference_v<T>, T, T&>;

  using pointer = value_type*;

  sync_wait_task_promise() : reset_event_{nullptr}, value_{nullptr}, exception_{nullptr} {}

  reference get_value()
  {
    if (exception_)
      std::rethrow_exception(exception_);
    return static_cast<reference>(*value_);
  }

  void start(simple_manual_reset_event* reset_event)
  {
    IOL_ASSERT(reset_event);
    reset_event_ = reset_event;
    coroutine_handle::from_promise(*this).resume();
  }

  sync_wait_task<T> get_return_object();

  std::suspend_always initial_suspend() { return {}; }

  auto final_suspend() noexcept { return sync_wait_notify_awaiter{}; }

  template <typename U>
  auto yield_value(U&& u)
  {
    value_ = std::addressof(u);
    return final_suspend();
  }

  void return_void()
  {
    // this should never happen!
    IOL_ASSERT(false);
  }

  void unhandled_exception() { exception_ = std::current_exception(); }

private:

  simple_manual_reset_event* reset_event_;

  pointer value_;

  std::exception_ptr exception_;
};

template <>
class sync_wait_task_promise<void>
{

  friend sync_wait_notify_awaiter;

public:

  using coroutine_handle = std::coroutine_handle<sync_wait_task_promise<void>>;

  sync_wait_task_promise() : reset_event_{nullptr}, exception_{nullptr} {}

  void get_value()
  {
    if (exception_)
      std::rethrow_exception(exception_);
  }

  void start(simple_manual_reset_event* reset_event)
  {
    IOL_ASSERT(reset_event);
    reset_event_ = reset_event;
    coroutine_handle::from_promise(*this).resume();
  }

  sync_wait_task<void> get_return_object();

  std::suspend_always initial_suspend() { return {}; }

  auto final_suspend() noexcept { return sync_wait_notify_awaiter{}; }

  void return_void() {}

  void unhandled_exception() { exception_ = std::current_exception(); }

private:

  simple_manual_reset_event* reset_event_;

  std::exception_ptr exception_;
};

template <typename T>
class sync_wait_task
{

  template <typename U>
  friend class sync_wait_task_promise;

public:

  using promise_type = sync_wait_task_promise<T>;

  sync_wait_task() : handle_{nullptr} {}

  sync_wait_task(sync_wait_task const&) = delete;

  sync_wait_task& operator=(sync_wait_task const&) = delete;

  ~sync_wait_task()
  {
    if (handle_)
      handle_.destroy();
  }

  void start(simple_manual_reset_event* event) { handle_.promise().start(event); }

  T get_value() { return handle_.promise().get_value(); }

private:

  sync_wait_task(sync_wait_coroutine_handle<T> handle) : handle_{handle} {}

  sync_wait_coroutine_handle<T> handle_;
};

template <typename T>
sync_wait_task<T> sync_wait_task_promise<T>::get_return_object()
{
  return {coroutine_handle::from_promise(*this)};
}

inline sync_wait_task<void> sync_wait_task_promise<void>::get_return_object()
{
  return coroutine_handle::from_promise(*this);
}

template <typename Ret, typename Awaitable>
requires std::is_same_v<Ret, void>
auto make_sync_wait_task(Awaitable&& awaitable) -> sync_wait_task<void>
{
  co_await std::forward<Awaitable>(awaitable);
}

template <typename Ret, typename Awaitable>
requires(!std::is_same_v<Ret, void>) auto make_sync_wait_task(Awaitable&& awaitable)
    -> sync_wait_task<Ret>
{
  co_yield co_await std::forward<Awaitable>(awaitable);
}

}  // namespace iol::detail

#endif  // IOL_DETAIL_SYNC_WAIT_TASK_HPP
