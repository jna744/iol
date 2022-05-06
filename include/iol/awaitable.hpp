#ifndef IOL_AWAITABLE_HPP
#define IOL_AWAITABLE_HPP

#include <iol/detail/config.hpp>

//

#include <coroutine>
#include <exception>
#include <utility>
#include <variant>

#if !IOL_SYMMETRIC_TRANSFER
#include <atomic>
#endif

namespace iol
{

template <typename = void>
class awaitable;

namespace detail
{

class awaitable_promise_base
{

  struct final_awaitable {

    bool await_ready() const noexcept { return false; }

#if IOL_SYMMETRIC_TRANSFER
    template <typename Promise>
    std::coroutine_handle<> await_suspend(
        std::coroutine_handle<Promise> awaitable_coroutine_handle) noexcept
    {
      // returning the continuation will resume it
      auto& promise = awaitable_coroutine_handle.promise();
      return promise.continuation_;
    }
#else
    template <typename Promise>
    void await_suspend(std::coroutine_handle<Promise> awaitable_coroutine_handle) noexcept
    {
      auto& promise = awaitable_coroutine_handle.promise();
      if (promise.state_.exchange(true, std::memory_order_acq_rel)) {
        promise.continuation_.resume();
      }
    }
#endif
    void await_resume() const noexcept {}
  };

public:

  awaitable_promise_base() : continuation_{nullptr} {}

#if IOL_SYMMETRIC_TRANSFER
  void set_continuation(std::coroutine_handle<> continuation)
  {
    continuation_ = continuation;
  }
#else
  bool try_set_continuation(std::coroutine_handle<> continuation)
  {
    continuation_ = continuation;
    return !state_.exchange(true, std::memory_order_acq_rel);
  }
#endif

  /* Coroutine members */

  std::suspend_always initial_suspend() noexcept
  {
    return {};
  }

  final_awaitable final_suspend() noexcept
  {
    // The final suspend continues execution of the continuation
    return {};
  }

private:

  std::coroutine_handle<> continuation_;

#if !IOL_SYMMETRIC_TRANSFER
  std::atomic<bool> state_;
#endif
};

template <typename T>
class awaitable_promise final : public awaitable_promise_base
{

  using value_type = std::conditional_t<std::is_reference_v<T>, std::reference_wrapper<T>, T>;

  using storage_type = std::variant<std::monostate, value_type, std::exception_ptr>;

  enum { empty, value, error };

public:

  using awaitable_promise_base::awaitable_promise_base;

  awaitable<T> get_return_object();

  template <typename U>
  requires requires(storage_type& s, U&& u) { s = std::forward<U>(u); }
  void return_value(U&& u) { storage_ = std::forward<U>(u); }

  void unhandled_exception() { storage_ = std::current_exception(); }

  T& get_value() &
  {
    IOL_ASSERT(storage_.index() != empty);
    if (storage_.index() == error)
      std::rethrow_exception(std::get<error>(storage_));
    return std::get<value>(storage_);
  }

  T&& get_value() &&
  {
    IOL_ASSERT(storage_.index() != empty);
    if (storage_.index() == error)
      std::rethrow_exception(std::get<error>(storage_));
    return std::get<value>(std::move(storage_));
  }

private:

  storage_type storage_;
};

template <>
class awaitable_promise<void> final : public awaitable_promise_base
{

public:

  using awaitable_promise_base::awaitable_promise_base;

  awaitable<void> get_return_object();

  void return_void() {}

  void unhandled_exception() { exception_ = std::current_exception(); }

  void get_value()
  {
    if (exception_)
      std::rethrow_exception(exception_);
  }

private:

  std::exception_ptr exception_;
};

}  // namespace detail

template <typename T>
class [[nodiscard]] awaitable
{

  friend detail::awaitable_promise<T>;

  class initial_awaitable
  {

    using coroutine_handle = std::coroutine_handle<detail::awaitable_promise<T>>;

  public:

    initial_awaitable(coroutine_handle handle) : handle_{handle} {}

    bool await_ready() { return !handle_ || handle_.done(); }

#if IOL_SYMMETRIC_TRANSFER
    std::coroutine_handle<> await_suspend(std::coroutine_handle<> continuation)
    {
      handle_.promise().set_continuation(continuation);
      return handle_;
    }
#else
    bool await_suspend(std::coroutine_handle<> continuation)
    {
      handle_.resume();
      return handle_.promise().try_set_continuation(continuation);
    }
#endif

  protected:

    coroutine_handle handle_;
  };

public:

  using promise_type = detail::awaitable_promise<T>;

  awaitable() noexcept : handle_{nullptr} {}

  awaitable(awaitable&& other) noexcept : handle_{std::exchange(other.handle_, nullptr)} {}

  awaitable& operator=(awaitable&& other) noexcept
  {
    if (this != &other) {
      auto temp(std::move(other));
      swap(temp);
    }
    return *this;
  }

  ~awaitable()
  {
    if (handle_)
      handle_.destroy();
  }

  void swap(awaitable<T>& other) noexcept
  {
    using std::swap;
    swap(handle_, other.handle_);
  }

  bool is_ready() const noexcept
  {
    return !handle_ || handle_.done();
  }

  auto operator co_await() const&
  {
    struct temp_awaitable final : public initial_awaitable {
      using initial_awaitable::initial_awaitable;
      decltype(auto) await_resume()
      {
        IOL_ASSERT(this->handle_);
        return this->handle_.promise().get_value();
      }
    };
    return temp_awaitable{handle_};
  }

  auto operator co_await() const&&
  {
    struct temp_awaitable final : public initial_awaitable {
      using initial_awaitable::initial_awaitable;
      decltype(auto) await_resume()
      {
        IOL_ASSERT(this->handle_);
        return std::move(this->handle_.promise()).get_value();
      }
    };
    return temp_awaitable{handle_};
  }

private:

  awaitable(std::coroutine_handle<promise_type> handle) : handle_{handle} {}

  std::coroutine_handle<promise_type> handle_;
};

template <typename T>
void swap(awaitable<T>& lhs, awaitable<T>& rhs) noexcept
{
  return lhs.swap(rhs);
}

namespace detail
{

template <typename T>
awaitable<T> awaitable_promise<T>::get_return_object()
{
  return awaitable<T>{std::coroutine_handle<awaitable_promise<T>>::from_promise(*this)};
}

inline awaitable<void> awaitable_promise<void>::get_return_object()
{
  return awaitable<void>{std::coroutine_handle<awaitable_promise<void>>::from_promise(*this)};
}
}  // namespace detail

}  // namespace iol

#endif  // IOL_AWAITABLE_HPP
