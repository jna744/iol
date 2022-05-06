#include <atomic>
#include <iol/static_thread_pool.hpp>

namespace
{

namespace local
{

using namespace iol;

struct thread_storage {

  explicit thread_storage(static_thread_pool* p_id)
    : operation_queue{}, operation_count{0}, previous_storage{top}, pool_id{p_id}
  {
    top = this;
  }

  ~thread_storage() { top = previous_storage; }

  detail::operation_queue operation_queue;
  std::size_t             operation_count;

  thread_storage*     previous_storage;
  static_thread_pool* pool_id;

  inline static thread_local thread_storage* top = nullptr;
};

}  // namespace local

}  // namespace

namespace iol
{

void static_thread_pool::schedule_coro_operation::invoke_impl(
    void* owner, detail::operation_base* base)
{
  auto* this_ = static_cast<schedule_coro_operation*>(base);
  if (owner) {
    this_->continuation_.resume();
  }
}

static_thread_pool::static_thread_pool(std::size_t n_threads)
  : running_{true}, work_count_{1}, main_operation_queue_{}, mut_{}, cv_{}, threads_{}
{

  n_threads = n_threads ? n_threads : 1;
  try {
    for (auto i = 0; i < n_threads; ++i)
      threads_.emplace_back(&static_thread_pool::attach, this);
  } catch (...) {
    {
      std::unique_lock<std::mutex> lock{mut_};
      running_.store(false, std::memory_order_relaxed);
    }
    cv_.notify_all();
    for (auto& t : threads_)
      if (t.joinable())
        t.join();
    throw;
  }
}

static_thread_pool::~static_thread_pool()
{
  {
    std::unique_lock<std::mutex> lock{mut_};
    running_.store(false, std::memory_order_relaxed);
  }
  cv_.notify_all();
  for (auto& t : threads_)
    if (t.joinable())
      t.join();
}

void static_thread_pool::attach()
{
  local::thread_storage storage{this};

  auto const invoke_local = [&, owner = this]
  {
    auto* op = storage.operation_queue.deque().release();
    storage.operation_count = 0;
    op->invoke(owner, op);
    return storage.operation_count == 1;
  };

  auto const is_running = [&]
  {
    return running_.load(std::memory_order_relaxed) &&
           work_count_.load(std::memory_order_relaxed) > 0;
  };

  std::unique_lock<std::mutex> lock{mut_};

  while (true) {

    cv_.wait(lock, [&] { return !is_running() || !main_operation_queue_.empty(); });

    if (!is_running())
      return;

    auto*      op = main_operation_queue_.deque().release();
    bool const more_ops = !main_operation_queue_.empty();

    lock.unlock();

    if (more_ops)
      cv_.notify_one();

    op->invoke(this, op);

    if (storage.operation_count == 1) {
      while (invoke_local()) {
        if (!running_.load(std::memory_order_relaxed))
          return;
      }
    }

    // count is > 1
    if (storage.operation_count) {
      work_count_.fetch_add(
          std::exchange(storage.operation_count, 0) - 1, std::memory_order_relaxed);
    } else if (work_count_.fetch_sub(1, std::memory_order_acq_rel) - 1 == 0) {
      cv_.notify_all();
      return;
    }

    lock.lock();
    main_operation_queue_.enqueue(std::move(storage.operation_queue));
  }
}

void static_thread_pool::stop()
{
  running_.store(false, std::memory_order_relaxed);
  cv_.notify_all();
}

void static_thread_pool::wait()
{
  std::unique_lock<std::mutex> lock{mut_};
  auto                         threads{std::move(threads_)};
  lock.unlock();

  if (!threads.empty()) {
    work_count_.fetch_sub(1, std::memory_order_relaxed);
    cv_.notify_all();
    for (auto& t : threads)
      if (t.joinable())
        t.join();
  }
}

bool static_thread_pool::running_in_this_thread() const noexcept
{
  auto* storage_ptr = local::thread_storage::top;
  while (storage_ptr)
    if ((storage_ptr++)->pool_id == this)
      return true;
  return false;
}

void static_thread_pool::enqueue_operation(detail::operation_ptr operation) noexcept
{
  {
    std::unique_lock<std::mutex> lock{mut_};
    main_operation_queue_.enqueue(std::move(operation));
    work_count_.fetch_add(1, std::memory_order_relaxed);
  }
  cv_.notify_one();
}

void static_thread_pool::enqueue_continuation(detail::operation_ptr operation) noexcept
{
  auto* storage = local::thread_storage::top;
  IOL_ASSERT(storage && storage->pool_id == this);
  storage->operation_queue.enqueue(std::move(operation));
  ++storage->operation_count;
}

}  // namespace iol
