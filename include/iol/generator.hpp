#ifndef IOL_GENERATOR_HPP
#define IOL_GENERATOR_HPP

#include <coroutine>
#include <iterator>
#include <utility>

namespace iol
{

template <typename T>
struct [[nodiscard]] generator {

  struct promise_type {

    using coroutine_handle = std::coroutine_handle<promise_type>;

    using value_type = std::remove_reference_t<T>;

    using reference_type = std::conditional_t<std::is_reference_v<T>, T, T&>;

    using pointer_type = value_type*;

    promise_type() : value_{nullptr} {}

    generator get_return_object() { return generator{coroutine_handle::from_promise(*this)}; }

    std::suspend_always initial_suspend() { return {}; }

    std::suspend_always final_suspend() noexcept { return {}; }

    template <typename U>
    std::suspend_always yield_value(U&& t)
    {
      value_ = std::addressof(t);
      return {};
    }

    void await_transform() = delete;

    [[noreturn]] void unhandled_exception() { throw; }

    reference_type value() const noexcept { return static_cast<reference_type>(*value_); }

  private:

    pointer_type value_;
  };

  constexpr generator() noexcept : handle_{nullptr} {}

  generator(generator const&) = delete;

  generator(generator&& other) noexcept : handle_{other.handle_} { other.handle_ = nullptr; }

  generator& operator=(generator const&) = delete;

  generator& operator=(generator other) noexcept
  {
    other.swap(*this);
    return *this;
  }

  ~generator()
  {
    if (handle_)
      handle_.destroy();
  }

  class iterator
  {

  public:

    using coroutine_handle = std::coroutine_handle<promise_type>;

    using value_type = typename promise_type::value_type;

    using reference = typename promise_type::reference_type;

    using pointer = typename promise_type::pointer_type;

    using iterator_category = std::input_iterator_tag;

    using difference_type = std::ptrdiff_t;

    iterator() noexcept : handle_{nullptr} {}

    iterator& operator++() noexcept
    {
      handle_.resume();
      return *this;
    }

    void operator++(int) noexcept { (void)++(*this); }

    reference operator*() const { return handle_.promise().value(); }

    pointer operator->() const { return &handle_.promise().value(); }

    friend bool operator==(iterator const& iter, std::default_sentinel_t) noexcept
    {
      return !iter.handle_ || iter.handle_.done();
    }

    friend bool operator==(std::default_sentinel_t end, iterator const& iter) noexcept { return iter == end; }

    friend bool operator!=(iterator const& iter, std::default_sentinel_t end) noexcept { return !(iter == end); }

    friend bool operator!=(std::default_sentinel_t end, iterator const& iter) noexcept { return !(iter == end); }

  private:

    friend generator;

    iterator(coroutine_handle handle) : handle_{handle} {}

    coroutine_handle handle_;
  };

  iterator begin()
  {
    if (handle_ && !handle_.done())
      handle_.resume();
    return {handle_};
  }

  std::default_sentinel_t end() const { return {}; }

  void swap(generator& other) noexcept
  {
    using std::swap;
    swap(handle_, other.handle_);
  }

private:

  using coroutine_handle = std::coroutine_handle<promise_type>;

  explicit generator(coroutine_handle handle) : handle_{std::move(handle)} {}

  coroutine_handle handle_;
};

template <typename T>
void swap(generator<T>& lhs, generator<T>& rhs) noexcept
{
  lhs.swap(rhs);
}

}  // namespace iol

#endif  // IOL_GENERATOR_HPP
