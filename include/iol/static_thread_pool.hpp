#ifndef IOL_STATIC_THREAD_POOL_HPP
#define IOL_STATIC_THREAD_POOL_HPP

#include <iol/detail/allocation_utility.hpp>
#include <iol/detail/config.hpp>
#include <iol/detail/operation_base.hpp>
#include <iol/detail/operation_queue.hpp>
#include <iol/get_allocator.hpp>

//

#include <atomic>
#include <condition_variable>
#include <coroutine>
#include <mutex>
#include <thread>
#include <vector>

namespace iol
{

class static_thread_pool
{

  struct schedule_coro_operation : detail::operation_base {

    schedule_coro_operation() : schedule_coro_operation(nullptr) {}

    bool await_ready() const noexcept { return (!pool_ || pool_->running_in_this_thread()); }

    void await_suspend(std::coroutine_handle<> continuation) noexcept
    {
      IOL_ASSERT(pool_);
      continuation_ = continuation;
      auto op = detail::operation_ptr{this};
      if (pool_->running_in_this_thread())
        pool_->enqueue_continuation(std::move(op));
      else
        pool_->enqueue_operation(std::move(op));
    }

    void await_resume() const noexcept {}

  private:

    friend static_thread_pool;
    schedule_coro_operation(static_thread_pool* pool)
      : detail::operation_base{invoke_impl, {}}, pool_{pool}, continuation_{nullptr}
    {
    }

    static void invoke_impl(void*, detail::operation_base* base);

    static_thread_pool*     pool_;
    std::coroutine_handle<> continuation_;
  };

  template <typename Allocator, typename Function>
  struct thread_pool_operation : public detail::operation_base {

    Allocator allocator;
    Function  function;

    template <typename Fn>
    thread_pool_operation(Allocator const& alloc, Fn&& fn)
      : detail::operation_base{
        [](auto* owner, auto* base) {
          auto cleanup = [](auto* this_) noexcept {
            detail::allocation_utility::destroy_and_delete(this_->allocator, this_);
          };
          auto this_ = std::unique_ptr<thread_pool_operation, decltype(cleanup)>{static_cast<thread_pool_operation*>(base), cleanup};
          Function fn{std::move(this_->function)};
          this_.reset();
          if (owner) {
            fn();
          }
        }, {}}, allocator{alloc}, function{std::forward<Fn>(fn)}
    {
    }
  };

public:

  static_thread_pool() : static_thread_pool(std::thread::hardware_concurrency()) {}

  explicit static_thread_pool(std::size_t n_threads);

  static_thread_pool(static_thread_pool&&) = delete;

  static_thread_pool& operator=(static_thread_pool&&) = delete;

  ~static_thread_pool();

  schedule_coro_operation schedule() noexcept { return {this}; }

  void attach();

  void stop();

  void wait();

  bool running_in_this_thread() const noexcept;

  template <typename Function>
  void post(Function&& function)
  {
    auto allocator = get_allocator(function);
    auto operation = detail::allocation_utility::make_operation<
        thread_pool_operation<allocator_t<Function>, std::remove_cvref_t<Function>>>(
        allocator, std::forward<Function>(function));
    enqueue_operation(std::move(operation));
  }

  template <typename Function>
  void defer(Function&& function)
  {
    auto allocator = get_allocator(function);
    auto operation = detail::allocation_utility::make_operation<
        thread_pool_operation<allocator_t<Function>, std::remove_cvref_t<Function>>>(
        allocator, std::forward<Function>(function));
    if (running_in_this_thread())
      enqueue_continuation(std::move(operation));
    else
      enqueue_operation(std::move(operation));
  }

private:

  void enqueue_operation(detail::operation_ptr operation) noexcept;

  /*
   * pre-condition: running_in_this_thread()
   * */
  void enqueue_continuation(detail::operation_ptr operation) noexcept;

  std::atomic_bool   running_;
  std::atomic_size_t work_count_;

  detail::operation_queue main_operation_queue_;

  std::mutex               mut_;
  std::condition_variable  cv_;
  std::vector<std::thread> threads_;
};

}  // namespace iol

#endif  // IOL_STATIC_THREAD_POOL_HPP
